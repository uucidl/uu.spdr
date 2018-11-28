/* basic automated tests */
/* @language: c99 */

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

#include "../include/spdr/spdr-private.h"

#include <stdint.h>
#include "../src/spdr_types.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void bucket_init(struct SPDR_Bucket *bucket, size_t buffer_size);

static int equal_zstring(char const* a, char const *b) {
    return strcmp(a, b) == 0;
}

static void hexdump(unsigned char* bytes, size_t bytes_n, unsigned int offset_base) {
    unsigned char* bytes_l = bytes + bytes_n;
    unsigned int row_offset = offset_base;
    printf("      ");
    {
        unsigned int n = 0;
        while(n < 8) {
            printf(" %02x", n);
            ++n;
        }
        printf(" ");
        while(n < 16) {
            printf(" %02x", n);
            ++n;
        }
        printf("\n");
    }
    while (bytes != bytes_l) {
        int n = 8;
        printf("0x%04x", row_offset);
        while (n-- && bytes != bytes_l) {
            printf(" %02X", *bytes); ++bytes;
        }
        printf(" ");
        n = 8;
        while (n-- && bytes != bytes_l) {
            printf(" %02X", *bytes); ++bytes;
        }
        printf ("\n");
        row_offset += 16;
    }
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    struct SPDR_Context spdr_context;
    memset(&spdr_context, 0, sizeof spdr_context);
    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        struct SPDR_Bucket* bucket;
        size_t size = sizeof *bucket + 64*(sizeof (struct SPDR_Block));
        bucket = calloc(1, size);
        bucket_init(bucket, size);
        spdr_context.buckets[i] = bucket;
    }
    uu_spdr_record_1(&spdr_context,
                     "<cat>", "<name>", SPDR_EVENT,
                     (struct SPDR_Event_Arg){
                     "<arg0.key>",
                     SPDR_INT,
                     { .i = 42 },
                     });

    /* print record layout */ {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextended-offsetof"
        printf("SPDR_Event layout:\n");
#define OP(m) printf("%20s: [0x%02x,0x%02x)\n", #m, \
        (unsigned int) offsetof(struct SPDR_Event, m), \
        (unsigned int) (offsetof(struct SPDR_Event, m) + \
                        sizeof ((struct SPDR_Event*)0)->m))
        OP(ts_ticks);
        OP(pid);
        OP(tid);
        OP(cat);
        OP(name);
        OP(phase);
        OP(str_count);
        OP(int_count);
        OP(float_count);
        OP(str_args[0].key);
        OP(str_args[0].value);
        OP(str_args[1].key);
        OP(str_args[1].value);
        OP(str_args[2].key);
        OP(str_args[2].value);
        OP(int_args[0].key);
        OP(int_args[0].value);
        OP(int_args[1].key);
        OP(int_args[1].value);
        OP(int_args[2].key);
        OP(int_args[2].value);
        OP(float_args[0].key);
        OP(float_args[0].value);
        OP(float_args[1].key);
        OP(float_args[1].value);
        OP(float_args[2].key);
        OP(float_args[2].value);
#pragma clang diagnostic pop
#undef OP
        printf("---\n");
        fflush(stdout);
    }

    size_t event_count = 0;
    for (size_t bucket_i = 0; bucket_i < BUCKET_COUNT; ++bucket_i) {
        struct SPDR_Bucket* bucket = spdr_context.buckets[bucket_i];
        size_t block_n = AO_load_acquire(&bucket->blocks_next);
        for (size_t block_i = 0; block_i < block_n; ++block_i) {
            struct SPDR_Block* block = bucket->blocks + block_i;
            if (block->type == EVENT_BLOCK) {
                struct SPDR_Event* event = &block->data.event;
                printf("Event:\n");
                hexdump((unsigned char*)event, sizeof *event, 0);
                printf("---\n");
                fflush(stdout);
                assert(equal_zstring(event->cat, "<cat>"));
                assert(equal_zstring(event->name, "<name>"));
                assert(event->phase == 'I');
                assert(event->str_count == 0);
                assert(event->int_count == 1);
                assert(event->float_count == 0);
                assert(equal_zstring(event->int_args[0].key, "<arg0.key>"));
                assert(event->int_args[0].value == 42);
                ++event_count;
            }
        }
    }

    assert(event_count == 1);

    for (size_t i = 0; i < BUCKET_COUNT; i++) {
        struct SPDR_Bucket** bucket_pos = spdr_context.buckets + i;
        free(*bucket_pos);
        *bucket_pos = NULL;
    }
    return 0;
}

#if defined(__APPLE__)
#include "TargetConditionals.h"
#if defined(TARGET_OS_MAC)
#include "../src/spdr_osx_unit.c"
#endif
#elif defined(_WIN32)
#include "../src/spdr_win32_unit.c"
#elif defined(_WIN64)
#include "../src/spdr_win64_unit.c"
#elif defined(__linux__) || defined(__freebsd__)
#include "../src/spdr_posix_unit.c"
#endif



