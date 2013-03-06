/* -*- coding: utf-8; -*- */

#include "../include/spdr.h"
#include "../deps/libatomic_ops-7.2/src/atomic_ops.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "spdr-internal.h"
#include "chars.h"
#include "clock.h"
#include "allocator.h"
#include "murmur.h"

#include "clock_type.h"
#include "allocator_type.h"

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
	struct { const char* key; double value; }      float_args[2];
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

struct BucketAllocator
{
	struct Allocator super;
	struct Bucket* bucket;
};

struct Bucket
{
	struct BucketAllocator allocator;
	int                    blocks_capacity;
	AO_t                   blocks_next;
	struct Block           blocks[1];
};

struct SpdrAllocator
{
	struct Allocator super;
	struct spdr* spdr;
};

enum {
	BUCKET_COUNT_BITS = 3,
	BUCKET_COUNT      = 2 << BUCKET_COUNT_BITS,
	BUCKET_COUNT_MASK = BUCKET_COUNT - 1
};

struct spdr
{
	int                  tracing_p;
	struct Clock*        clock;
	char                 clock_buffer[sizeof (struct Clock)];
	unsigned long long (*clock_fn)(void* user_data);
	void*                clock_user_data;

	void               (*log_fn) (const char* line, void* user_data);
	void*                log_user_data;

	struct SpdrAllocator clock_allocator;

	/*
	 * the struct is immediately followed by the rest of the
	 * buffer (the arena), where data will be allocated from
	 */
	int arena_size;
	int arena_offset;

	/**
	 * Pointers to the arena
	 */
	struct Bucket*  buckets[BUCKET_COUNT];
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

static void* bucket_alloc(struct Allocator* allocator, size_t size)
{
	if (size > INT_MAX) {
		return NULL;
	}

	{
		struct Bucket* bucket = ((struct BucketAllocator*) allocator)->bucket;
		int nblocks = 1 + (int) size / sizeof(struct Block);
		struct Block* first_block = growblocks_until(bucket->blocks, bucket->blocks_capacity, &bucket->blocks_next, nblocks);

		if (!first_block) {
			return NULL;
		}

		first_block->type = STR_BLOCK;

		return &first_block->data.chars;
	}
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

static void bucket_init(struct Bucket* bucket, int buffer_size)
{
	bucket->allocator.super.alloc = bucket_alloc;
	bucket->allocator.super.free  = spdr_free;
	bucket->allocator.bucket      = bucket;

	bucket->blocks_capacity = 1 + (buffer_size - sizeof *bucket) / sizeof bucket->blocks[0];
	AO_store(&bucket->blocks_next, 0);
}

extern int spdr_init(struct spdr **context_ptr, void* buffer, size_t _buffer_size)
{
	const struct spdr null = { 0 };
	int const buffer_size = _buffer_size > INT_MAX ? INT_MAX : (int) _buffer_size;

	struct spdr* context;

	int arena_offset;
	void*  arena;
	int arena_size;

	if (buffer_size < sizeof *context) {
		return -1;
	}

	context = buffer;
	*context = null;

	/* arena memory starts right after the buffer, aligned to 64 bytes */
	arena_offset = sizeof *context;
	{
		size_t abs_arena_offset = ((char*) buffer - ((char*) 0)) + arena_offset;
		size_t aligned_arena_offset = (1 + (abs_arena_offset>>6))<<6;

		arena_offset += (int) (aligned_arena_offset - abs_arena_offset);
	}

	arena = ((char*) buffer) + arena_offset;
	arena_size = buffer_size - arena_offset;

	if (arena_size <= 0) {
		return -1;
	}

	context->clock_allocator.super.alloc = spdr_clock_alloc;
	context->clock_allocator.super.free  = spdr_free;
	context->clock_allocator.spdr        = context;

	if (clock_init(&context->clock, &context->clock_allocator.super) < 0) {
		return -1;
	}

	context->arena_size   = arena_size;
	context->arena_offset = arena_offset;

	{
		int const n = BUCKET_COUNT;
		int i;
		int bucket_size = arena_size / n;
		bucket_size = (bucket_size >> 6) << 6;

		if (bucket_size < sizeof *context->buckets[0]) {
			return -1;
		}

		for (i = 0; i < n; i++) {
			context->buckets[i] = (void*) (((char*) arena) + i * bucket_size);
			bucket_init(context->buckets[i], bucket_size);
		}

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
	int i;

	for (i = 0; i < BUCKET_COUNT; i++) {
		AO_store(&context->buckets[i]->blocks_next, 0);
	}
}

extern struct spdr_capacity spdr_capacity(struct spdr* context)
{
	struct spdr_capacity cap = {0};
	int i;

	for (i = 0; i < BUCKET_COUNT; i++) {
		struct Bucket const* bucket = context->buckets[i];
		cap.count    += AO_load(&bucket->blocks_next);
		cap.capacity += bucket->blocks_capacity;
	}

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
	char line[2048];
	struct Chars buffer = { 0 };
	const char* prefix = "";

	buffer.chars = line;
	buffer.capacity = sizeof line;

	if (with_newlines_p) {
		prefix = "\n";
	}

	chars_catsprintf (&buffer,
			  "%s%llu %u %llu",
			  prefix,
			  event->ts_microseconds,
			  event->pid,
			  event->tid);

	chars_catsprintf(&buffer, " \"");
	chars_catjsonstr(&buffer, event->cat);
	chars_catsprintf(&buffer, "\" \"");
	chars_catjsonstr(&buffer, event->name);
	chars_catsprintf(&buffer, "\"");
	chars_catsprintf(&buffer, " \"%c\"", event->phase);

	for (i = 0; i < event->str_count; i++) {
		chars_catsprintf(&buffer,
				 " \"%s\" \"",
				 event->str_args[i].key);
		chars_catjsonstr(&buffer, event->str_args[i].value);
		chars_catsprintf(&buffer, "\"");
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
	char buffer[2048];
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
	uint64_t const key[] = { e->pid, e->tid, e->ts_microseconds };
	int const bucket_i = (murmurhash3_32(key, sizeof key, 0x4356))  & BUCKET_COUNT_MASK;
	struct Bucket* bucket = context->buckets[bucket_i];
	struct Event* ep = growlog_until(bucket->blocks, bucket->blocks_capacity, &bucket->blocks_next);
	int i;

	if (!ep) {
		return;
	}

	memcpy(ep, e, sizeof *e);

	for (i = 0; i < ep->str_count; i++) {
		const char* const str = ep->str_args[i].value;
		/* take copy of strings */
		size_t n = strlen(str) + 1;

		char* new_str = allocator_alloc(&bucket->allocator.super, n);

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

static int event_timecmp(void const * const _a, void const * const _b)
{
	struct Event const * const * ap = _a;
	struct Event const * const * bp = _b;

	struct Event const * a = *ap;
	struct Event const * b = *bp;

	if (a->ts_microseconds == b->ts_microseconds) {
		if (a->pid == b->pid) {
			if (a->tid == b->tid) {
				/*
				 * compare pointers, since within the same buckets,
				 * they are in order.
				 */
				return a < b ? -1 : 1;
			}

			return a->tid < b->tid ? -1 : 1;
		}

		return a->pid < b->pid ? -1 : 1;
	}

	return a->ts_microseconds < b->ts_microseconds ? -1 : 1;
}

extern void spdr_report(struct spdr *context,
			enum spdr_report_type report_type,
			void (*print_fn) (const char* text, void* user_data),
			void* user_data)
{
	size_t records_per_bucket[BUCKET_COUNT];
	size_t block_count = 0;

	struct Event const** events;
	int events_n;
	int i;

	if (!print_fn) {
		return;
	}

	/* blocks all further recording */
	for (i = 0; i < BUCKET_COUNT; i++) {
		struct Bucket* const bucket = context->buckets[i];
		records_per_bucket[i] = AO_load_acquire(&bucket->blocks_next);
		AO_store(&bucket->blocks_next, bucket->blocks_capacity);
		block_count += records_per_bucket[i];
	}

	events = malloc(block_count * sizeof events[0]);
	events_n = 0;

	for (i = 0; i < BUCKET_COUNT; i++) {
		struct Bucket const * const bucket = context->buckets[i];
		size_t j;

		for (j = 0; j < records_per_bucket[i]; j++) {
			const struct Block* block = &bucket->blocks[j];
			if (block->type == EVENT_BLOCK) {
				events[events_n++] = &block->data.event;
			}
		}
	}

	qsort((void*)events, events_n, sizeof events[0], event_timecmp);

	if (SPDR_PLAIN_REPORT == report_type) {
		int i;

		for (i = 0; i < events_n; i++) {
			event_log(context, events[i], print_fn, user_data, T(with_newlines));
		}
	} else if (SPDR_CHROME_REPORT == report_type) {
		int i;
		const char* prefix = "";

		print_fn("{\"traceEvents\":[", user_data);
		for (i = 0; i < events_n; i++) {
			log_json(events[i], prefix, print_fn, user_data);
			prefix = ",";
		}
		print_fn("]}", user_data);
	}

	free ((void*)events);
	events = NULL;
}
