/* -*- coding: utf-8; -*- */

#include "spdr.h"
#include "spdr-internal.h"

#include <timer_lib/timer.h>
#include <libatomic_ops-7.2/src/atomic_ops.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "chars.h"

#define T(x) (!0)

struct Event
{
	uint64_t           ts_microseconds;
	uint32_t           pid;
	uint64_t           tid;
	const char*        cat;
	const char*        name;
	enum uu_spdr_type  phase;
	int                arg_count;
	struct uu_spdr_arg args[2];
};

struct Block
{
	enum { EVENT_BLOCK, STR_BLOCK } type;
	int count; // extra block count;
	union BlockData {
		struct Event event;
		char         chars[sizeof (struct Event)];
	} data;
};

struct spdr
{
	int          tracing_p;
	AO_t         blocks_next;
	timer        timer;
	void       (*log_fn) (const char* line, void* user_data);
	void*        log_user_data;
	size_t       blocks_capacity;
	struct Block blocks[1];
};

/**
* Grow the memory buffer until capacity is reached.
*
* @return a block or NULL if the capacity has been reached.
*/
static struct Block* growblocks_until(struct Block* blocks, size_t blocks_capacity, volatile AO_t* next, int nblocks)
{
	AO_t i = AO_fetch_and_add_acquire(next, nblocks);

	if (i >= blocks_capacity) {
		AO_fetch_and_add_release(next, -nblocks);
		return NULL;
	}

	blocks[i].count = nblocks - 1;
	return &blocks[i];
}

/**
* Allocate an event block
*/
static struct Event* growlog_until(struct Block* blocks, size_t blocks_capacity, volatile AO_t* next)
{
	struct Block* block = growblocks_until(blocks, blocks_capacity, next, 1);
	if (!block) {
		return NULL;
	}

	block->type = EVENT_BLOCK;

	return &block->data.event;
}

extern int spdr_init(struct spdr **context_ptr, void* buffer, size_t buffer_size)
{
	const struct spdr null = { 0 };
	struct spdr* context;

	if (timer_lib_initialize() < 0) {
		return -1;
	}

	if (buffer_size < sizeof *context) {
		return -1;
	}

	context = buffer;
	*context = null;

	context->blocks_capacity = 1 + (buffer_size - sizeof *context) / (sizeof *context->blocks);
	AO_store(&context->blocks_next, 0);

	timer_initialize(&context->timer);

	*context_ptr = context;

	return 0;
}

extern void spdr_deinit(struct spdr** context_ptr)
{
	timer_lib_shutdown();
	*context_ptr = NULL;
}

extern void spdr_reset(struct spdr* context)
{
	AO_store(&context->blocks_next, 0);
}

extern struct spdr_capacity spdr_capacity(struct spdr* context)
{
	struct spdr_capacity cap;

	cap.count    = AO_load(&context->blocks_next);
	cap.capacity = context->blocks_capacity;

	return cap;
}


/**
* Provide your logging function if you want a trace stream to be produced.
*/
void spdr_set_log_fn(struct spdr *context,
					 void (*log_fn) (const char* line, void* user_data),
					 void* user_data)
{
	context->log_fn        = log_fn;
	context->log_user_data = user_data;
}

/**
* Activates the recording of traces (off by default)
*/
extern void spdr_enable_trace(struct spdr *context, int traceon)
{
	context->tracing_p = traceon;
}

int uu_spdr_musttrace(const struct spdr *context)
{
	return context->tracing_p;
}

extern struct uu_spdr_arg uu_spdr_arg_make_int(const char* key, int value)
{
	struct uu_spdr_arg arg = { key, SPDR_INT, { 0 } };
	arg.value.i = value;
	return arg;
}

extern struct uu_spdr_arg uu_spdr_arg_make_double(const char* key, double value)
{
	struct uu_spdr_arg arg = { key, SPDR_FLOAT, { 0 } };
	arg.value.d = value;
	return arg;
}

extern struct uu_spdr_arg uu_spdr_arg_make_str(const char* key, const char* value)
{
	struct uu_spdr_arg arg = { key, SPDR_STR, { 0 } };
	arg.value.str = value;
	return arg;
}

static void event_make(struct spdr* context,
struct Event* event,
	const char* cat,
	const char* name,
	enum uu_spdr_type type)
{
	tick_t ticks = timer_elapsed_ticks (&context->timer, 0);
	uint64_t ticks_per_micros = timer_ticks_per_second (&context->timer) / 1000000;

	event->ts_microseconds = ticks / ticks_per_micros;
	event->pid   = uu_spdr_get_pid();
	event->tid   = uu_spdr_get_tid();
	event->cat   = cat;
	event->name  = name;
	event->phase = type;
	event->arg_count = 0;
}

static void event_add_arg(struct spdr* context, struct Event* event, struct uu_spdr_arg arg)
{
	if (arg.type == SPDR_STR) {
		/* take copy of strings */
		int n = strlen(arg.value.str) + 1;
		int nblocks = 1 + n / (sizeof (union BlockData));
		struct Block* first_block = growblocks_until(context->blocks, context->blocks_capacity, &context->blocks_next, nblocks);

		if (!first_block) {
			arg.value.str = "<Out of arg. memory>";
		} else {
			first_block->type = STR_BLOCK;
			memcpy(&first_block->data.chars, arg.value.str, n);

			arg.value.str = first_block->data.chars;
		}
	}

	event->args[event->arg_count++] = arg;
}

static void event_log(const struct spdr* context,
					  const struct Event* event,
					  void (*print_fn) (const char* string, void *user_data),
					  void* user_data,
					  int with_newlines_p)
{
	int i;
	char line[1024];
	struct Chars buffer = { 0 };
	const char* prefix = "";

	buffer.chars = line;
	buffer.capacity = sizeof line;

	if (with_newlines_p) {
		prefix = "\n";
	}


	chars_catsprintf (&buffer,
		"%s%llu %u %llu \"%s\" \"%s\" \"%c\"",
		prefix,
		event->ts_microseconds,
		event->pid,
		event->tid,
		event->cat,
		event->name,
		event->phase);

	for (i = 0; i < event->arg_count; i++) {
		const struct uu_spdr_arg* arg = &event->args[i];

		switch (arg->type) {
		case SPDR_INT:
			chars_catsprintf(&buffer,
				" \"%s\" %d",
				arg->key,
				arg->value.i);
			break;
		case SPDR_FLOAT:
			chars_catsprintf(&buffer,
				" \"%s\" %f",
				arg->key,
				arg->value.d);
			break;
		case SPDR_STR:
			chars_catsprintf(&buffer,
				" \"%s\" \"%s\"",
				arg->key,
				arg->value.str);
			break;
		}
	}

	if (0 == buffer.error)
	{
		print_fn(buffer.chars, user_data);
	}
}


static void log_json(const struct Event* e,
					 const char* prefix,
					 void (*print_fn) (const char* string, void* user_data),
					 void* user_data)
{
	int i;
	const char* arg_prefix = "";
	char buffer[512];
	struct Chars string = { 0 };
	string.chars = buffer;
	string.capacity = sizeof buffer;

	chars_catsprintf(&string,
		"%s{\"ts\":%llu,\"pid\":%u,\"tid\":%llu",
		prefix,
		e->ts_microseconds,
		e->pid,
		e->tid);

	chars_catsprintf(&string, ",\"cat\":\"");
	chars_catjsonstr(&string, e->cat);
	chars_catsprintf(&string, "\"");
	chars_catsprintf(&string, ",\"name\":\"");
	chars_catjsonstr(&string, e->name);
	chars_catsprintf(&string, "\"");
	chars_catsprintf(&string, ",\"ph\":\"%c\",\"args\":{", e->phase);


	for (i = 0; i < e->arg_count; i++) {
		const struct uu_spdr_arg* arg = &e->args[i];

		switch (arg->type) {
		case SPDR_INT:
			chars_catsprintf(&string,
				"%s\"%s\":%d",
				arg_prefix,
				arg->key,
				arg->value.i);
			break;
		case SPDR_FLOAT:
			chars_catsprintf(&string,
				"%s\"%s\":%f",
				arg_prefix,
				arg->key,
				arg->value.d);
			break;
		case SPDR_STR:
			chars_catsprintf(&string,
				"%s\"%s\":\"",
				arg_prefix,
				arg->key);
			chars_catjsonstr(&string, arg->value.str);
			chars_catsprintf(&string, "\"");
			break;
		}
		arg_prefix = ",";
	}

	chars_catsprintf(&string, "}}");

	if (0 == string.error)
	{
		print_fn(string.chars, user_data);
	}
}
extern void uu_spdr_record(struct spdr *context,
						   const char* cat,
						   const char* name,
						   enum uu_spdr_type type)
{
	struct Event* e = growlog_until(context->blocks, context->blocks_capacity, &context->blocks_next);
	if (!e) {
		return;
	}

	event_make (context, e, cat, name, type);

	if (context->log_fn) {
		event_log (context, e, context->log_fn, context->log_user_data, !T(with_newlines));
	}
}

extern void uu_spdr_record_1(struct spdr *context,
							 const char* cat,
							 const char* name,
							 enum uu_spdr_type type,
struct uu_spdr_arg arg0)
{
	struct Event* e = growlog_until(context->blocks, context->blocks_capacity, &context->blocks_next);
	if (!e) {
		return;
	}

	event_make (context, e, cat, name, type);
	event_add_arg (context, e, arg0);
	if (context->log_fn) {
		event_log (context, e, context->log_fn, context->log_user_data, !T(with_newlines));
	}
}

extern void uu_spdr_record_2(struct spdr *context,
							 const char* cat,
							 const char* name,
							 enum uu_spdr_type type,
struct uu_spdr_arg arg0,
struct uu_spdr_arg arg1)
{
	struct Event* e = growlog_until(context->blocks, context->blocks_capacity, &context->blocks_next);
	if (!e) {
		return;
	}

	event_make (context, e, cat, name, type);
	event_add_arg (context, e, arg0);
	event_add_arg (context, e, arg1);
	if (context->log_fn) {
		event_log (context, e, context->log_fn, context->log_user_data, !T(with_newlines));
	}
}

void spdr_report(struct spdr *context,
				 enum spdr_report_type report_type,
				 void (*print_fn) (const char* text, void* user_data),
				 void* user_data)
{
	struct spdr_capacity cap = spdr_capacity(context);
	size_t i;

	if (!print_fn) {
		return;
	}

	if (SPDR_PLAIN_REPORT == report_type) {
		for (i = 0; i < cap.count; i++) {
			const struct Block *block = &context->blocks[i];
			if (block->type == EVENT_BLOCK)
			{
				event_log(context, &block->data.event, print_fn, user_data, T(with_newlines));
			}

			i += block->count;
		}
	} else if (SPDR_CHROME_REPORT == report_type) {
		const char* prefix = "";

		print_fn("{\"traceEvents\":[", user_data);
		for (i = 0; i < cap.count; i++) {
			const struct Block *block = &context->blocks[i];
			if (block->type == EVENT_BLOCK)
			{
				log_json(&block->data.event, prefix, print_fn, user_data);
			}

			prefix = ",";
			i += block->count;
		}
		print_fn("]}", user_data);
	}
}
