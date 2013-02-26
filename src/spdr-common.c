/* -*- coding: utf-8; -*- */

#include "../include/spdr.h"
#include "../deps/libatomic_ops-7.2/src/atomic_ops.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "spdr-internal.h"
#include "chars.h"
#include "clock.h"
#include "allocator_type.h"
#include "allocator.h"

/**
 *  "documented" truth value, use like:
 *
 * T(use_defibrilator)
 */
#define T(x) (!0)

struct Event
{
	uint64_t           ts_microseconds;
	uint32_t           pid;
	uint64_t           tid;
	const char*        cat;
	const char*        name;
	enum uu_spdr_type  phase;
	int8_t             str_count;
	int8_t             int_count;
	int8_t             float_count;
	struct { const char* key; const char* value; } str_args[2];
	struct { const char* key; int value; }         int_args[2];
	struct { const char* key; float value; }       float_args[2];
};

struct Block
{
	enum { EVENT_BLOCK, STR_BLOCK } type;

	/*
	 * extra block count.
	 *
	 * Whenever a block is not sufficient, contiguous blocks of
	 * memory are allocated of exactly:
	 *   count * sizeof(struct Block)
	 */
	int count;
	union BlockData {
		struct Event event;
		char         chars[sizeof (struct Event)];
	} data;
};

struct SpdrAllocator
{
	struct Allocator super;
	struct spdr* spdr;
};

struct spdr
{
	int                  tracing_p;
	struct Clock*        clock;
	char                 clock_buffer[32];
	unsigned long long (*clock_fn)(void* user_data);
	void*                clock_user_data;

	void               (*log_fn) (const char* line, void* user_data);
	void*                log_user_data;

	struct SpdrAllocator clock_allocator;
	struct SpdrAllocator arena_allocator;
	size_t               blocks_capacity;

	/* a small buffer between read areas and write areas */
	char buffer[1024];

	AO_t                 blocks_next;
	struct Block         blocks[1];
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

static void* spdr_alloc(struct Allocator* allocator, size_t size)
{
	struct spdr* context = ((struct SpdrAllocator*) allocator)->spdr;

	int nblocks = 1 + size / sizeof(struct Block);

	struct Block* first_block = growblocks_until(context->blocks, context->blocks_capacity, &context->blocks_next, nblocks);

	if (!first_block) {
		return NULL;
	}

	first_block->type = STR_BLOCK;

	return &first_block->data.chars;
}

static void spdr_free(struct Allocator* _, void* ptr)
{
	(void) _;
	(void) ptr;

	/* no op */
}

static void* spdr_clock_alloc(struct Allocator* allocator, size_t size)
{
	struct spdr* context = ((struct SpdrAllocator*) allocator)->spdr;
	if (size > sizeof context->clock_buffer) {
		return NULL;
	}

	return &context->clock_buffer;
}

extern int spdr_init(struct spdr **context_ptr, void* buffer, size_t buffer_size)
{
	const struct spdr null = { 0 };
	struct spdr* context;

	if (buffer_size < sizeof *context) {
		return -1;
	}

	context = buffer;
	*context = null;

	context->blocks_capacity = 1 + (buffer_size - sizeof *context) / (sizeof *context->blocks);
	AO_store(&context->blocks_next, 0);

	context->arena_allocator.super.alloc = spdr_alloc;
	context->arena_allocator.super.free  = spdr_free;
	context->arena_allocator.spdr        = context;

	context->clock_allocator.super.alloc = spdr_clock_alloc;
	context->clock_allocator.super.free  = spdr_free;
	context->clock_allocator.spdr        = context;

	if (clock_init(&context->clock, &context->clock_allocator.super) < 0) {
		return -1;
	}

	*context_ptr = context;

	return 0;
}

extern void spdr_deinit(struct spdr** context_ptr)
{
	clock_deinit(&(*context_ptr)->clock);
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

extern void spdr_set_clock_microseconds_fn(struct spdr *context,
		     unsigned long long (*clock_microseconds_fn)(void* user_data),
		     void *user_data)
{
	context->clock_fn        = clock_microseconds_fn;
	context->clock_user_data = user_data;
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

static void event_make(
	struct spdr* context,
	struct Event* event,
	const char* cat,
	const char* name,
	enum uu_spdr_type type)
{
	if (context->clock_fn) {
		event->ts_microseconds = context->clock_fn(context->clock_user_data);
	} else {
		event->ts_microseconds = clock_microseconds(context->clock);
	}
	event->pid   = uu_spdr_get_pid();
	event->tid   = uu_spdr_get_tid();
	event->cat   = cat;
	event->name  = name;
	event->phase = type;
	event->str_count = 0;
	event->int_count = 0;
	event->float_count = 0;
}

static void event_add_arg(struct spdr* context, struct Event* event, struct uu_spdr_arg arg)
{
	int i;

	switch (arg.type) {
	case SPDR_STR:
		i = event->str_count++;
		event->str_args[i].key   = arg.key;
		event->str_args[i].value = arg.value.str;
		break;
	case SPDR_INT:
		i = event->int_count++;
		event->int_args[i].key   = arg.key;
		event->int_args[i].value = arg.value.i;
		break;
	case SPDR_FLOAT:
		i = event->float_count++;
		event->float_args[i].key   = arg.key;
		event->float_args[i].value = arg.value.d;
		break;
	}
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

	for (i = 0; i < event->str_count; i++) {
		chars_catsprintf(&buffer,
				 " \"%s\" \"%s\"",
				 event->str_args[i].key,
				 event->str_args[i].value);
	}

	for (i = 0; i < event->int_count; i++) {
		chars_catsprintf(&buffer,
				 " \"%s\" %d",
				 event->int_args[i].key,
				 event->int_args[i].value);
	}

	for (i = 0; i < event->float_count; i++) {
		chars_catsprintf(&buffer,
				 " \"%s\" %f",
				 event->float_args[i].key,
				 event->float_args[i].value);
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


	for (i = 0; i < e->str_count; i++) {
		chars_catsprintf(&string,
				 "%s\"%s\":\"",
				 arg_prefix,
				 e->str_args[i].key);
		chars_catjsonstr(&string, e->str_args[i].value);
		chars_catsprintf(&string, "\"");

		arg_prefix = ",";
	}

	for (i = 0; i < e->int_count; i++) {
		chars_catsprintf(&string,
				 "%s\"%s\":%d",
				 arg_prefix,
				 e->int_args[i].key,
				 e->int_args[i].value);
		arg_prefix = ",";
	}

	for (i = 0; i < e->float_count; i++) {
		chars_catsprintf(&string,
				 "%s\"%s\":%f",
				 arg_prefix,
				 e->float_args[i].key,
				 e->float_args[i].value);
		arg_prefix = ",";
	}

	chars_catsprintf(&string, "}}");

	if (0 == string.error)
	{
		print_fn(string.chars, user_data);
	}
}

static void record_event(struct spdr* context, struct Event* e)
{
	struct Event* ep = growlog_until(context->blocks, context->blocks_capacity, &context->blocks_next);
	int i;

	if (!ep) {
		return;
	}

	memcpy(ep, e, sizeof *e);

	for (i = 0; i < ep->str_count; i++) {
		const char* const str = ep->str_args[i].value;
		/* take copy of strings */
		int n = strlen(str) + 1;

		char* new_str = allocator_alloc(&context->arena_allocator.super, n);

		if (!new_str) {
			ep->str_args[i].value = "<Out of arg. memory>";
		} else {
			memcpy(new_str, str, n);
			ep->str_args[i].value = new_str;
		}
	}
}

extern void uu_spdr_record(struct spdr *context,
			   const char* cat,
			   const char* name,
			   enum uu_spdr_type type)
{
	struct Event e;
	event_make (context, &e, cat, name, type);

	if (context->log_fn) {
		event_log (context, &e, context->log_fn, context->log_user_data, !T(with_newlines));
	}

	record_event(context, &e);
}

extern void uu_spdr_record_1(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     struct uu_spdr_arg arg0)
{
	struct Event e;

	event_make (context, &e, cat, name, type);
	event_add_arg (context, &e, arg0);
	if (context->log_fn) {
		event_log (context, &e, context->log_fn, context->log_user_data, !T(with_newlines));
	}

	record_event(context, &e);
}

extern void uu_spdr_record_2(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     struct uu_spdr_arg arg0,
			     struct uu_spdr_arg arg1)
{
	struct Event e;
	event_make (context, &e, cat, name, type);
	event_add_arg (context, &e, arg0);
	event_add_arg (context, &e, arg1);
	if (context->log_fn) {
		event_log (context, &e, context->log_fn, context->log_user_data, !T(with_newlines));
	}

	record_event(context, &e);
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

	/* blocks all further recording */
	AO_store(&context->blocks_next, context->blocks_capacity);

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
				prefix = ",";
			}

			i += block->count;
		}
		print_fn("]}", user_data);
	}
}
