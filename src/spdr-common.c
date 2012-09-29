#include "spdr.h"
#include "spdr_internal.h"
#include <timer_lib/timer.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

struct spdr
{
	int tracing_p;
	timer timer;
	void (*log_fn) (const char* line);
};

static void null_log_fn (const char* line)
{
	// no op
}

extern int spdr_init(struct spdr **pcontext)
{
	if (timer_lib_initialize() < 0) {
		return -1;
	}

	struct spdr* context = calloc (sizeof *context, 1);
	if (!context) {
		return -1;
	}

	timer_initialize(&context->timer);
	context->log_fn = null_log_fn;

	*pcontext = context;

	return 0;
}

extern void spdr_deinit(struct spdr* context)
{
	free (context);
	timer_lib_shutdown();
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

extern void uu_spdr_record(struct spdr *context,
			   const char* cat,
			   const char* name,
			   enum uu_spdr_type type)
{
	tick_t ticks = timer_elapsed_ticks (&context->timer, 0);
	uint64_t ticks_per_micros = timer_ticks_per_second (&context->timer) / 1000000;
	uint64_t microseconds = ticks / ticks_per_micros;
	uint32_t pid = uu_spdr_get_pid();
	uint64_t tid = uu_spdr_get_tid();

	char line[256];
	snprintf (line, sizeof line,
		  "%llu %u %llu \"%s\" \"%s\" \"%c\"",
		  microseconds, pid, tid, cat, name, type);

	context->log_fn(line);
}

extern void uu_spdr_record_1(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     const char* key0,
			     const char* value0)
{
	uu_spdr_record(context, cat, name, type);
}

extern void uu_spdr_record_2(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     const char* key0,
			     const char* value0,
			     const char* key1,
			     const char* value1)
{
	uu_spdr_record(context, cat, name, type);
}
