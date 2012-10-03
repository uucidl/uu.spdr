/* -*- coding: utf-8; -*- */
#include "spdr.h"
#include "spdr_internal.h"
#include <timer_lib/timer.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct spdr
{
	int tracing_p;
	timer timer;
	void (*log_fn) (const char* line);
};

struct event
{
	uint64_t ts_microseconds;
	uint32_t pid;
	uint64_t tid;
	const char* cat;
	const char* name;
	enum uu_spdr_type phase;
	int arg_count;
	struct spdr_arg args[2];
};

extern int spdr_init(struct spdr **context_ptr)
{
	struct spdr* context;

	if (timer_lib_initialize() < 0) {
		return -1;
	}

	context = calloc (sizeof *context, 1);
	if (!context) {
		return -1;
	}

	timer_initialize(&context->timer);

	*context_ptr = context;

	return 0;
}

extern void spdr_deinit(struct spdr** context_ptr)
{
	timer_lib_shutdown();
	free (*context_ptr);
	*context_ptr = NULL;
}

/**
 * Provide your logging function if you want a trace stream to be produced.
 */
void spdr_set_log_fn(struct spdr *context,
		     void (*log_fn) (const char* line))
{
	context->log_fn = log_fn;
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

extern struct spdr_arg spdr_arg_make_int(const char* key, int value)
{
	struct spdr_arg arg = { key, SPDR_INT, { 0 } };
	arg.value.i = value;
	return arg;
}

extern struct spdr_arg spdr_arg_make_double(const char* key, double value)
{
	struct spdr_arg arg = { key, SPDR_FLOAT, { 0 } };
	arg.value.d = value;
	return arg;
}

extern struct spdr_arg spdr_arg_make_str(const char* key, const char* value)
{
	struct spdr_arg arg = { key, SPDR_STR, { 0 } };
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

static void event_add_arg(struct event* event, struct spdr_arg arg)
{
	event->args[event->arg_count++] = arg;
}

static void event_log(struct spdr* context, struct event* event)
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
		int line_n = strlen(line);
		char *ptr = line + line_n;
		int size  = (sizeof line) - line_n;
		struct spdr_arg* arg = &event->args[i];
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

	context->log_fn(line);
}

extern void uu_spdr_record(struct spdr *context,
			   const char* cat,
			   const char* name,
			   enum uu_spdr_type type)
{
	struct event e;
	event_make (context, &e, cat, name, type);
	if (context->log_fn) {
		event_log (context, &e);
	}
}

extern void uu_spdr_record_1(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     struct spdr_arg arg0)
{
	struct event e;
	event_make (context, &e, cat, name, type);
	event_add_arg (&e, arg0);
	if (context->log_fn) {
		event_log (context, &e);
	}
}

extern void uu_spdr_record_2(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     struct spdr_arg arg0,
			     struct spdr_arg arg1)
{
	struct event e;
	event_make (context, &e, cat, name, type);
	event_add_arg (&e, arg0);
	event_add_arg (&e, arg1);
	if (context->log_fn) {
		event_log (context, &e);
	}
}
