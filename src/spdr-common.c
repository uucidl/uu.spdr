/* -*- coding: utf-8; -*- */

#include "spdr.h"
#include "spdr-internal.h"

#include <timer_lib/timer.h>
#include <libatomic_ops-7.2/src/atomic_ops.h>

#include <stdlib.h>
#include <stdio.h>
#include "uu-string.h"

struct event
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

struct spdr
{
	int          tracing_p;
	AO_t         log_next;
	timer        timer;
	void       (*log_fn) (const char* line, void* user_data);
	void*        log_user_data;
	size_t       log_capacity;
	struct event log[1];
};

/**
 * Grow the log until capacity is reached.
 *
 * @return an event or NULL if the capacity has been reached.
 */
static struct event* growlog_until(struct event* logs, size_t log_capacity, volatile AO_t* next)
{
	AO_t i = AO_fetch_and_add1_acquire(next);

	if (i >= log_capacity) {
		AO_fetch_and_sub1_release(next);
		return NULL;
	}

	return &logs[i];
}

extern int spdr_init(struct spdr **context_ptr, void* buffer, size_t buffer_size)
{
	struct spdr* context;

	if (timer_lib_initialize() < 0) {
		return -1;
	}

	if (buffer_size < sizeof *context) {
		return -1;
	}

	context = buffer;
	memset(context, 0, sizeof *context);

	context->log_capacity = 1 + (buffer_size - sizeof *context) / sizeof (struct event);
	AO_store(&context->log_next, 0);

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
	AO_store(&context->log_next, 0);
}

extern struct spdr_capacity spdr_capacity(struct spdr* context)
{
	struct spdr_capacity cap;

	cap.count = AO_load(&context->log_next);
	cap.capacity = context->log_capacity;

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
		       struct event* event,
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

static void event_add_arg(struct event* event, struct uu_spdr_arg arg)
{
	event->args[event->arg_count++] = arg;
}

static void event_log(const struct spdr* context,
		      const struct event* event,
		      void (*log_fn) (const char* line, void *user_data),
		      void* user_data)
{
	int i;
	char line[256];

	snprintf (line, sizeof line,
		  "%llu %u %llu \"%s\" \"%s\" \"%c\"",
		  event->ts_microseconds,
		  event->pid,
		  event->tid,
		  event->cat,
		  event->name,
		  event->phase);

	for (i = 0; i < event->arg_count; i++) {
		const struct uu_spdr_arg* arg = &event->args[i];

		size_t line_n = strlen(line);
		char *ptr = line + line_n;
		size_t size  = (sizeof line) - line_n;


		switch (arg->type) {
		case SPDR_INT:
			snprintf(ptr, size,
				 " \"%s\" %d",
				 arg->key,
				 arg->value.i);
			break;
		case SPDR_FLOAT:
			snprintf(ptr, size,
				 " \"%s\" %f",
				 arg->key,
				 arg->value.d);
			break;
		case SPDR_STR:
			snprintf(ptr, size,
				 " \"%s\" \"%s\"",
				 arg->key,
				 arg->value.str);
			break;
		}
	}

	log_fn(line, user_data);
}

extern void uu_spdr_record(struct spdr *context,
			   const char* cat,
			   const char* name,
			   enum uu_spdr_type type)
{
	struct event* e = growlog_until(context->log, context->log_capacity, &context->log_next);
	if (!e) {
		return;
	}

	event_make (context, e, cat, name, type);

	if (context->log_fn) {
		event_log (context, e, context->log_fn, context->log_user_data);
	}
}

extern void uu_spdr_record_1(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     struct uu_spdr_arg arg0)
{
	struct event* e = growlog_until(context->log, context->log_capacity, &context->log_next);
	if (!e) {
		return;
	}

	event_make (context, e, cat, name, type);
	event_add_arg (e, arg0);
	if (context->log_fn) {
		event_log (context, e, context->log_fn, context->log_user_data);
	}
}

extern void uu_spdr_record_2(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     struct uu_spdr_arg arg0,
			     struct uu_spdr_arg arg1)
{
	struct event* e = growlog_until(context->log, context->log_capacity, &context->log_next);
	if (!e) {
		return;
	}

	event_make (context, e, cat, name, type);
	event_add_arg (e, arg0);
	event_add_arg (e, arg1);
	if (context->log_fn) {
		event_log (context, e, context->log_fn, context->log_user_data);
	}
}

static log_json(const struct event* e,
	       const char* prefix,
	       void (*log_fn) (const char* line, void* user_data),
	       void* user_data)
{
	/* TO BE IMPLEMENTED */
}

void spdr_report(struct spdr *context,
		 enum spdr_report_type report_type,
		 void (*log_fn) (const char* line, void* user_data),
		 void* user_data)
{
	struct spdr_capacity cap = spdr_capacity(context);
	size_t i;

	if (!log_fn) {
		return;
	}

	if (SPDR_PLAIN_REPORT == report_type) {
		for (i = 0; i < cap.count; i++) {
			const struct event *e = &context->log[i];

			event_log(context, e, log_fn, user_data);
		}
	} else if (SPDR_CHROME_REPORT == report_type) {
		const char* prefix = "";

		log_fn("{\"traceEvents\":[", user_data);
		for (i = 0; i < cap.count; i++) {
			const struct event *e = &context->log[i];

			log_json(e, prefix, log_fn, user_data);

			prefix = ",";
		}
		log_fn("]}", user_data);
	}
}
