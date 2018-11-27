/* -*- coding: utf-8; -*- */

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wbad-function-cast"
#pragma clang diagnostic ignored "-Wcast-qual"
#endif
#include "../deps/libatomic_ops-7.6.6/src/atomic_ops.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "../include/spdr/spdr.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "chars.h"
#include "clock.h"
#include "float.h"
#include "inlines.h"
#include "murmur.h"
#include "spdr-internal.h"
#include "spdr_types.h"

/**
 *  "documented" truth value, use like:
 *
 * SPDR_T(use_defibrilator)
 */
#define SPDR_T(x) (!0)

#define SPDR_PRIu64 "%" PRINTF_INT64_MODIFIER "u"

/**
 * Grow the memory buffer until capacity is reached.
 *
 * @return a block or NULL if the capacity has been reached.
 */
spdr_internal struct SPDR_Block *growblocks_until(struct SPDR_Block *blocks,
                                                  size_t blocks_capacity,
                                                  volatile AO_t *next,
                                                  size_t nblocks)
{
        intptr_t increment = (intptr_t)nblocks;
        AO_t i = AO_fetch_and_add_acquire(next, (AO_t)increment);

        if (i >= blocks_capacity) {
                AO_fetch_and_add_release(next, (AO_t)-increment);
                return NULL;
        }

        blocks[i].count = nblocks - 1;
        return &blocks[i];
}

/**
 * Allocate an event block
 */
spdr_internal struct SPDR_Event *growlog_until(struct SPDR_Block *blocks,
                                               size_t blocks_capacity,
                                               volatile AO_t *next)
{
        struct SPDR_Block *block =
            growblocks_until(blocks, blocks_capacity, next, 1);
        if (!block) {
                return NULL;
        }

        block->type = EVENT_BLOCK;

        return &block->data.event;
}

spdr_internal void *bucket_alloc(struct SPDR_Allocator *allocator, size_t size)
{
        if (size > INT_MAX) {
                return NULL;
        }

        {
                struct SPDR_Bucket *bucket =
                    ((struct SPDR_Bucket_Allocator *)allocator)->bucket;
                size_t nblocks = 1 + size / sizeof(struct SPDR_Block);
                struct SPDR_Block *first_block =
                    growblocks_until(bucket->blocks, bucket->blocks_capacity,
                                     &bucket->blocks_next, nblocks);

                if (!first_block) {
                        return NULL;
                }

                first_block->type = STR_BLOCK;

                return &first_block->data.chars;
        }
}

spdr_internal void spdr_free(struct SPDR_Allocator *_, void *ptr)
{
        (void)_;
        (void)ptr;

        /* no op */
}

spdr_internal void *spdr_clock_alloc(struct SPDR_Allocator *allocator,
                                     size_t size)
{
        struct SPDR_Context *context =
            ((struct SPDR_Main_Allocator *)allocator)->spdr;
        if (size > sizeof context->clock_buffer) {
                return NULL;
        }

        return &context->clock_buffer;
}

spdr_internal void bucket_init(struct SPDR_Bucket *bucket, size_t buffer_size)
{
        bucket->allocator.super.alloc = bucket_alloc;
        bucket->allocator.super.free = spdr_free;
        bucket->allocator.bucket = bucket;

        bucket->blocks_capacity =
            1 + (buffer_size - sizeof *bucket) / sizeof bucket->blocks[0];
        AO_store(&bucket->blocks_next, 0);
}

spdr_internal struct SPDR_Context spdr_make_null_context(void)
{
        struct SPDR_Context null_context;
        null_context.tracing_p = 0;
        null_context.clock = NULL;
        null_context.clock_fn = NULL;
        null_context.clock_user_data = NULL;
        null_context.log_fn = NULL;
        null_context.log_user_data = NULL;
        null_context.arena_size = 0;
        return null_context;
}

extern int
spdr_init(struct SPDR_Context **context_ptr, void *buffer, size_t buffer_size)
{
        struct SPDR_Context null_context = spdr_make_null_context();
        struct SPDR_Context *context;
        void *arena;
        uintptr_t buffer_last_pos;
        uintptr_t arena_pos;
        size_t arena_size;

        if (buffer_size < sizeof *context) {
                return -1;
        }

        context = SPDR_VOID_PTR_CAST(SPDR_Context, buffer);
        *context = null_context;

        buffer_last_pos = (uintptr_t)buffer + buffer_size;

        /* arena memory starts right after the buffer, aligned to 64 bytes */
        arena_pos = (((uintptr_t)buffer + sizeof *context + 64) &
                     (~((uintptr_t)(64 - 1))));
        if (arena_pos >= buffer_last_pos) {
                return -1;
        } else {
                arena_size = buffer_last_pos - arena_pos;
        }
        arena = (void *)arena_pos;
        context->clock_allocator.super.alloc = spdr_clock_alloc;
        context->clock_allocator.super.free = spdr_free;
        context->clock_allocator.spdr = context;

        if (clock_init(&context->clock, &context->clock_allocator.super) < 0) {
                return -1;
        }

        {
                size_t const n = BUCKET_COUNT;
                size_t i;
                size_t bucket_size = arena_size / n;
                bucket_size = (bucket_size >> 6) << 6;

                if (bucket_size < (int)sizeof *context->buckets[0]) {
                        return -1;
                }

                context->arena_size = arena_size;

                for (i = 0; i < n; i++) {
                        struct SPDR_Bucket *bucket = SPDR_VOID_PTR_CAST(
                            struct SPDR_Bucket,
                            (void *)(((char *)arena) + i * bucket_size));
                        context->buckets[i] = bucket;
                        bucket_init(bucket, bucket_size);
                }
        }

        *context_ptr = context;

        return 0;
}

extern void spdr_deinit(struct SPDR_Context **context_ptr)
{
        clock_deinit(&(*context_ptr)->clock);
        *context_ptr = NULL;
}

extern void spdr_reset(struct SPDR_Context *context)
{
        int i;

        for (i = 0; i < BUCKET_COUNT; i++) {
                AO_store(&context->buckets[i]->blocks_next, 0);
        }
}

extern struct SPDR_Capacity spdr_capacity(struct SPDR_Context *context)
{
        struct SPDR_Capacity cap;
        struct SPDR_Bucket **bucketp = &context->buckets[0];
        int bucket_count = BUCKET_COUNT;

        cap.count = 0;
        cap.capacity = 0;
        while (bucket_count--) {
                struct SPDR_Bucket const *bucket = *bucketp;
                cap.count += AO_load(&bucket->blocks_next);
                cap.capacity += bucket->blocks_capacity;
                ++bucketp;
        }

        return cap;
}

extern void spdr_set_clock_microseconds_fn(
    struct SPDR_Context *context,
    uint64_t (*clock_microseconds_fn)(void *user_data),
    void *user_data)
{
        context->clock_fn = clock_microseconds_fn;
        context->clock_user_data = user_data;
}

/**
 * Provide your logging function if you want a trace stream to be produced.
 */
void spdr_set_log_fn(struct SPDR_Context *context,
                     void (*log_fn)(const char *line, void *user_data),
                     void *user_data)
{
        context->log_fn = log_fn;
        context->log_user_data = user_data;
}

/**
 * Activates the recording of traces (off by default)
 *
 * FEATURE: tracing is activated and deactivated at runtime
 */
extern void spdr_enable_trace(struct SPDR_Context *context, int traceon)
{
        context->tracing_p = traceon;
}

extern int uu_spdr_musttrace(const struct SPDR_Context *context)
{
        /*  FEATURE: a null context is treated as a *never tracing* context */
        return context && context->tracing_p;
}

extern struct SPDR_Event_Arg uu_spdr_arg_make_int(const char *key, int value)
{
        struct SPDR_Event_Arg arg = {0, SPDR_INT, {0}};
        arg.key = key;
        arg.value.i = value;

        return arg;
}

extern struct SPDR_Event_Arg uu_spdr_arg_make_double(const char *key,
                                                     double value)
{
        struct SPDR_Event_Arg arg = {0, SPDR_FLOAT, {0}};
        arg.key = key;
        arg.value.d = value;

        return arg;
}

extern struct SPDR_Event_Arg uu_spdr_arg_make_str(const char *key,
                                                  const char *value)
{
        struct SPDR_Event_Arg arg = {0, SPDR_STR, {0}};
        arg.key = key;
        arg.value.str = value;

        return arg;
}

spdr_internal void event_make(struct SPDR_Context *context,
                              struct SPDR_Event *event,
                              const char *cat,
                              const char *name,
                              enum SPDR_Event_Type type)
{
        if (context->clock_fn) {
                event->ts_ticks = context->clock_fn(context->clock_user_data);
        } else {
                event->ts_ticks = clock_ticks(context->clock);
        }
        event->pid = uu_spdr_get_pid();
        event->tid = uu_spdr_get_tid();
        event->cat = cat;
        event->name = name;
        event->phase = type;
        event->str_count = 0;
        event->int_count = 0;
        event->float_count = 0;
}

spdr_internal void event_add_arg(struct SPDR_Event *event,
                                 struct SPDR_Event_Arg arg)
{
        int i;

        switch (arg.type) {
        case SPDR_STR:
                i = event->str_count++;
                event->str_args[i].key = arg.key;
                event->str_args[i].value = arg.value.str;
                break;
        case SPDR_INT:
                i = event->int_count++;
                event->int_args[i].key = arg.key;
                event->int_args[i].value = arg.value.i;
                break;
        case SPDR_FLOAT:
                i = event->float_count++;
                event->float_args[i].key = arg.key;
                event->float_args[i].value = arg.value.d;
                break;
        }
}

spdr_internal void event_log(const struct SPDR_Context *context,
                             const struct SPDR_Event *event,
                             void (*print_fn)(const char *string,
                                              void *user_data),
                             void *user_data,
                             int with_newlines_p)
{
        char line[2048];
        struct SPDR_Chars buffer = SPDR_Chars_NULL;
        const char *prefix = "";
        uint64_t ts_microseconds =
            context->clock_fn
                ? event->ts_ticks
                : clock_ticks_to_microseconds(context->clock, event->ts_ticks);

        buffer.chars = line;
        buffer.capacity = sizeof line;

        if (with_newlines_p) {
                prefix = "\n";
        }

        chars_catsprintf(&buffer, "%s" SPDR_PRIu64 " %u " SPDR_PRIu64 "",
                         prefix, ts_microseconds, event->pid, event->tid);

        chars_catsprintf(&buffer, " \"");
        chars_catjsonstr(&buffer, event->cat);
        chars_catsprintf(&buffer, "\" \"");
        chars_catjsonstr(&buffer, event->name);
        chars_catsprintf(&buffer, "\"");
        chars_catsprintf(&buffer, " \"%c\"", event->phase);

        {
                int arg_i;
                for (arg_i = 0; arg_i < event->str_count; arg_i++) {
                        chars_catsprintf(&buffer, " \"%s\" \"",
                                         event->str_args[arg_i].key);
                        chars_catjsonstr(&buffer, event->str_args[arg_i].value);
                        chars_catsprintf(&buffer, "\"");
                }
        }

        {
                int arg_i;
                for (arg_i = 0; arg_i < event->int_count; arg_i++) {
                        chars_catsprintf(&buffer, " \"%s\" %d",
                                         event->int_args[arg_i].key,
                                         event->int_args[arg_i].value);
                }
        }

        {
                int arg_i;
                for (arg_i = 0; arg_i < event->float_count; arg_i++) {
                        chars_catsprintf(&buffer, " \"%s\" %f",
                                         event->float_args[arg_i].key,
                                         event->float_args[arg_i].value);
                }
        }

        if (0 == buffer.error) {
                print_fn(buffer.chars, user_data);
        }
}

spdr_internal int has_non_json_arg(struct SPDR_Event const *const event,
                                   struct SPDR_Event_Arg *const first_arg)
{
        int i;

        for (i = 0; i < event->float_count; i++) {
                if (!float_isfinite(event->float_args[i].value)) {
                        *first_arg = UU_SPDR_FLOAT(event->float_args[i].key,
                                                   event->float_args[i].value);

                        return 1;
                }
        }

        return 0;
}

spdr_internal void log_json_arg_error(const struct SPDR_Context *context,
                                      const struct SPDR_Event *e,
                                      struct SPDR_Event_Arg *arg,
                                      const char *prefix,
                                      void (*print_fn)(const char *string,
                                                       void *user_data),
                                      void *user_data)
{
        uint64_t ts_microseconds =
            context->clock_fn
                ? e->ts_ticks
                : clock_ticks_to_microseconds(context->clock, e->ts_ticks);
        char buffer[2048];
        struct SPDR_Chars string = SPDR_Chars_NULL;
        char arg_value_buffer[256];
        struct SPDR_Chars arg_value_string = SPDR_Chars_NULL;
        const char *arg_prefix = "";

        string.chars = buffer;
        string.capacity = sizeof buffer;
        arg_value_string.chars = arg_value_buffer;
        arg_value_string.capacity = sizeof arg_value_buffer;

        chars_catsprintf(&string, "%s{\"ts\":" SPDR_PRIu64
                                  ",\"pid\":%u,\"tid\":" SPDR_PRIu64 "",
                         prefix, ts_microseconds, e->pid, e->tid);

        chars_catsprintf(&string, ",\"cat\":\"spdr-error\"");
        chars_catsprintf(&string, ",\"name\":\"arg-serialization\"");
        chars_catsprintf(&string, ",\"ph\":\"I\",\"args\":{");

        switch (arg->type) {
        case SPDR_INT:
                chars_catsprintf(&arg_value_string, "%d", arg->value.i);
                break;
        case SPDR_FLOAT:
                chars_catsprintf(&arg_value_string, "%f", arg->value.d);
                break;
        case SPDR_STR:
                chars_catsprintf(&arg_value_string, "%s", arg->value.str);
                break;
        }

        {
                size_t i;
                const char *arg_value = arg_value_string.error
                                            ? "<format-error>"
                                            : arg_value_string.chars;
                struct {
                        const char *key;
                        const char *value;
                } str_args[] = {
                    {"cat", 0}, {"name", 0}, {"failed-arg", 0}, {0, 0},
                };
                size_t str_args_n = sizeof str_args / sizeof *str_args;
                str_args[0].value = e->cat;
                str_args[1].value = e->name;
                str_args[2].value = arg->key;
                str_args[3].key = arg->key;
                str_args[3].value = arg_value;

                for (i = 0; i < str_args_n; i++) {
                        chars_catsprintf(&string, "%s\"%s\":\"", arg_prefix,
                                         str_args[i].key);
                        chars_catjsonstr(&string, str_args[i].value);
                        chars_catsprintf(&string, "\"");

                        arg_prefix = ",";
                }
        }

        chars_catsprintf(&string, "}}");

        if (0 == string.error) {
                print_fn(string.chars, user_data);
        }
}

spdr_internal void log_json(const struct SPDR_Context *context,
                            const struct SPDR_Event *e,
                            const char *prefix,
                            void (*print_fn)(const char *string,
                                             void *user_data),
                            void *user_data)
{
        uint64_t ts_microseconds =
            context->clock_fn
                ? e->ts_ticks
                : clock_ticks_to_microseconds(context->clock, e->ts_ticks);
        int i;
        const char *arg_prefix = "";
        int need_id = 0;
        uint32_t id = 0;
        char buffer[2048];
        struct SPDR_Chars string = SPDR_Chars_NULL;

        string.chars = buffer;
        string.capacity = sizeof buffer;
        {
                struct SPDR_Event_Arg first_arg;
                if (has_non_json_arg(e, &first_arg)) {
                        log_json_arg_error(context, e, &first_arg, prefix,
                                           print_fn, user_data);
                }
        }

        if (e->phase == SPDR_ASYNC_EVENT_BEGIN ||
            e->phase == SPDR_ASYNC_EVENT_STEP ||
            e->phase == SPDR_ASYNC_EVENT_END) {
                need_id = 1;
        }

        chars_catsprintf(&string, "%s{\"ts\":" SPDR_PRIu64
                                  ",\"pid\":%u,\"tid\":" SPDR_PRIu64 "",
                         prefix, ts_microseconds, e->pid, e->tid);

        chars_catsprintf(&string, ",\"cat\":\"");
        chars_catjsonstr(&string, e->cat);
        chars_catsprintf(&string, "\"");
        chars_catsprintf(&string, ",\"name\":\"");
        chars_catjsonstr(&string, e->name);
        chars_catsprintf(&string, "\"");
        chars_catsprintf(&string, ",\"ph\":\"%c\",\"args\":{", e->phase);

        for (i = 0; i < e->str_count; i++) {
                chars_catsprintf(&string, "%s\"%s\":\"", arg_prefix,
                                 e->str_args[i].key);
                chars_catjsonstr(&string, e->str_args[i].value);
                chars_catsprintf(&string, "\"");

                arg_prefix = ",";
        }

        for (i = 0; i < e->int_count; i++) {
                if (need_id && 0 == strcmp("id", e->int_args[i].key)) {
                        id = (uint32_t)e->int_args[i].value;
                        continue;
                }
                chars_catsprintf(&string, "%s\"%s\":%d", arg_prefix,
                                 e->int_args[i].key, e->int_args[i].value);
                arg_prefix = ",";
        }

        for (i = 0; i < e->float_count; i++) {
                if (!float_isfinite(e->float_args[i].value)) {
                        chars_catsprintf(&string, "%s\"%s\":0.0", arg_prefix,
                                         e->float_args[i].key);
                } else {
                        chars_catsprintf(&string, "%s\"%s\":%f", arg_prefix,
                                         e->float_args[i].key,
                                         e->float_args[i].value);
                }
                arg_prefix = ",";
        }

        chars_catsprintf(&string, "}");
        if (need_id) {
                chars_catsprintf(&string, ",\"id\": %d", id);
        }
        chars_catsprintf(&string, "}");

        if (0 == string.error) {
                print_fn(string.chars, user_data);
        }
}

spdr_internal int get_bucket_i(struct SPDR_Event const *const e)
{
        uint8_t key[sizeof e->tid + sizeof e->ts_ticks];
        {
                uint8_t *d = key;
                memcpy(d, &e->tid, sizeof e->tid);
                d += sizeof e->tid;
                memcpy(d, &e->ts_ticks, sizeof e->ts_ticks);
                d += sizeof e->ts_ticks;
        }
        return murmurhash3_32(key, sizeof key, 0x4356) & BUCKET_COUNT_MASK;
}

struct SPDR_EventAndBucket {
        struct SPDR_Event *event;
        struct SPDR_Bucket *bucket;
};

spdr_internal struct SPDR_EventAndBucket
growlog(struct SPDR_Context *const context, int const start_bucket_i)
{
        struct SPDR_EventAndBucket result;
        int i;

        /* allocate from main bucket and if it fails, try the other ones */
        for (i = 0; i < BUCKET_COUNT; i++) {
                int const bucket_i = (start_bucket_i + i) & BUCKET_COUNT_MASK;
                struct SPDR_Bucket *bucket = context->buckets[bucket_i];
                struct SPDR_Event *ep =
                    growlog_until(bucket->blocks, bucket->blocks_capacity,
                                  &bucket->blocks_next);
                if (ep) {
                        result.event = ep;
                        result.bucket = bucket;
                        return result;
                }
        }

        result.event = NULL;
        result.bucket = NULL;
        return result;
}

spdr_internal void record_event(struct SPDR_Context *context,
                                struct SPDR_Event *e)
{
        struct SPDR_EventAndBucket pair = growlog(context, get_bucket_i(e));
        struct SPDR_Event *ep = pair.event;
        struct SPDR_Bucket *bucket = pair.bucket;
        int i;

        if (!ep) {
                /* we've evidently reached full capacity */
                return;
        }

        memcpy(ep, e, sizeof *e);

        for (i = 0; i < ep->str_count; i++) {
                const char *const str = ep->str_args[i].value;
                /* take copy of strings */
                size_t n = strlen(str) + 1;

                char *new_str = SPDR_VOID_PTR_CAST(
                    char, allocator_alloc(&bucket->allocator.super, n));

                if (!new_str) {
                        ep->str_args[i].value = "<Out of arg. memory>";
                } else {
                        memcpy(new_str, str, n);
                        ep->str_args[i].value = new_str;
                }
        }
}

extern void uu_spdr_record(struct SPDR_Context *context,
                           const char *cat,
                           const char *name,
                           enum SPDR_Event_Type type)
{
        struct SPDR_Event e;
        event_make(context, &e, cat, name, type);

        if (context->log_fn) {
                event_log(context, &e, context->log_fn, context->log_user_data,
                          !SPDR_T(with_newlines));
        }

        record_event(context, &e);
}

extern void uu_spdr_record_1(struct SPDR_Context *context,
                             const char *cat,
                             const char *name,
                             enum SPDR_Event_Type type,
                             struct SPDR_Event_Arg arg0)
{
        struct SPDR_Event e;

        event_make(context, &e, cat, name, type);
        event_add_arg(&e, arg0);
        if (context->log_fn) {
                event_log(context, &e, context->log_fn, context->log_user_data,
                          !SPDR_T(with_newlines));
        }

        record_event(context, &e);
}

extern void uu_spdr_record_2(struct SPDR_Context *context,
                             const char *cat,
                             const char *name,
                             enum SPDR_Event_Type type,
                             struct SPDR_Event_Arg arg0,
                             struct SPDR_Event_Arg arg1)
{
        struct SPDR_Event e;
        event_make(context, &e, cat, name, type);
        event_add_arg(&e, arg0);
        event_add_arg(&e, arg1);
        if (context->log_fn) {
                event_log(context, &e, context->log_fn, context->log_user_data,
                          !SPDR_T(with_newlines));
        }

        record_event(context, &e);
}

extern void uu_spdr_record_3(struct SPDR_Context *context,
                             const char *cat,
                             const char *name,
                             enum SPDR_Event_Type type,
                             struct SPDR_Event_Arg arg0,
                             struct SPDR_Event_Arg arg1,
                             struct SPDR_Event_Arg arg2)
{
        struct SPDR_Event e;
        event_make(context, &e, cat, name, type);
        event_add_arg(&e, arg0);
        event_add_arg(&e, arg1);
        event_add_arg(&e, arg2);
        if (context->log_fn) {
                event_log(context, &e, context->log_fn, context->log_user_data,
                          !SPDR_T(with_newlines));
        }

        record_event(context, &e);
}

spdr_internal int event_timecmp(void const *const _a, void const *const _b)
{
        struct SPDR_Event const *const *ap =
            SPDR_VOID_PTR_CAST(struct SPDR_Event const *const, _a);
        struct SPDR_Event const *const *bp =
            SPDR_VOID_PTR_CAST(struct SPDR_Event const *const, _b);

        struct SPDR_Event const *a = *ap;
        struct SPDR_Event const *b = *bp;

        if (a->ts_ticks == b->ts_ticks) {
                if (a->pid == b->pid) {
                        if (a->tid == b->tid) {
                                /*
                                 * compare pointers, since within the same
                                 * buckets,
                                 * they are in order.
                                 */
                                return a < b ? -1 : 1;
                        }

                        return a->tid < b->tid ? -1 : 1;
                }

                return a->pid < b->pid ? -1 : 1;
        }

        return a->ts_ticks < b->ts_ticks ? -1 : 1;
}

extern void spdr_report(struct SPDR_Context *context,
                        enum SPDR_Report_Type report_type,
                        void (*print_fn)(const char *text, void *user_data),
                        void *user_data)
{
        size_t records_per_bucket[BUCKET_COUNT];
        size_t block_count = 0;

        struct SPDR_Event const **events;
        size_t events_n;
        int bucket_i;

        if (!print_fn) {
                return;
        }

        /* blocks all further recording */
        for (bucket_i = 0; bucket_i < BUCKET_COUNT; bucket_i++) {
                struct SPDR_Bucket *const bucket = context->buckets[bucket_i];
                records_per_bucket[bucket_i] =
                    AO_load_acquire(&bucket->blocks_next);
                AO_store(&bucket->blocks_next, bucket->blocks_capacity);
                block_count += records_per_bucket[bucket_i];
        }

        events = SPDR_VOID_PTR_CAST(struct SPDR_Event const *,
                                    malloc(block_count * sizeof events[0]));
        events_n = 0;

        for (bucket_i = 0; bucket_i < BUCKET_COUNT; bucket_i++) {
                struct SPDR_Bucket const *const bucket =
                    context->buckets[bucket_i];
                size_t j;

                for (j = 0; j < records_per_bucket[bucket_i]; j++) {
                        const struct SPDR_Block *block = &bucket->blocks[j];
                        if (block->type == EVENT_BLOCK) {
                                events[events_n++] = &block->data.event;
                        }
                }
        }

        qsort((void *)events, events_n, sizeof events[0], event_timecmp);

        if (SPDR_PLAIN_REPORT == report_type) {
                size_t event_i;

                for (event_i = 0; event_i < events_n; event_i++) {
                        event_log(context, events[event_i], print_fn, user_data,
                                  SPDR_T(with_newlines));
                }
        } else if (SPDR_CHROME_REPORT == report_type) {
                size_t event_i;
                const char *prefix = "";

                print_fn("{\"traceEvents\":[", user_data);
                for (event_i = 0; event_i < events_n; event_i++) {
                        log_json(context, events[event_i], prefix, print_fn,
                                 user_data);
                        prefix = ",";
                }
                print_fn("]", user_data);
                print_fn(",\"createdBy\":\"uu.spdr -- "
                         "http://github.com/uucidl/uu.spdr\"",
                         user_data);
                print_fn("}", user_data);
        }

        free((void *)events);
        events = NULL;
}

#undef SPDR_PRIu64
#undef SPDR_T
