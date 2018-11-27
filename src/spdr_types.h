#ifndef SPDR_TYPES_H
#define SPDR_TYPES_H

#include "uu-stdint.h"

#include "allocator_type.h"
#include "clock_type.h"

/**
 *  "documented" truth value, use like:
 *
 * SPDR_T(use_defibrilator)
 */
#define SPDR_T(x) (!0)

struct SPDR_Event {
        uint64_t ts_ticks;
        uint32_t pid;
        uint64_t tid;
        const char *cat;
        const char *name;
        enum SPDR_Event_Type phase;
        uint8_t str_count;
        uint8_t int_count;
        uint8_t float_count;
        struct {
                const char *key;
                const char *value;
        } str_args[3];
        struct {
                const char *key;
                int value;
        } int_args[3];
        struct {
                const char *key;
                double value;
        } float_args[3];
};

enum BlockType { EVENT_BLOCK, STR_BLOCK };

struct SPDR_Block {
        enum BlockType type;

        /*
         * extra block count.
         *
         * Whenever a block is not sufficient, contiguous blocks of
         * memory are allocated of exactly:
         *   count * sizeof(struct SPDR_Block)
         */
        size_t count;
        union BlockData {
                struct SPDR_Event event;
                char chars[sizeof(struct SPDR_Event)];
        } data;
};

struct SPDR_Bucket_Allocator {
        struct SPDR_Allocator super;
        struct SPDR_Bucket *bucket;
};

struct SPDR_Bucket {
        struct SPDR_Bucket_Allocator allocator;
        size_t blocks_capacity;
        AO_t blocks_next;
        struct SPDR_Block blocks[1];
};

struct SPDR_Main_Allocator {
        struct SPDR_Allocator super;
        struct SPDR_Context *spdr;
};

enum { BUCKET_COUNT_BITS = 3,
       BUCKET_COUNT = 2 << BUCKET_COUNT_BITS,
       BUCKET_COUNT_MASK = BUCKET_COUNT - 1 };

struct SPDR_Context {
        int tracing_p;
        struct SPDR_Clock *clock;
        char clock_buffer[sizeof(struct SPDR_Clock)];
        uint64_t (*clock_fn)(void *user_data);
        void *clock_user_data;

        void (*log_fn)(const char *line, void *user_data);
        void *log_user_data;

        struct SPDR_Main_Allocator clock_allocator;

        /*
         * the struct is immediately followed by the rest of the
         * buffer (the arena), where data will be allocated from
         */
        size_t arena_size;

        /**
         * Pointers to the arena
         */
        struct SPDR_Bucket *buckets[BUCKET_COUNT];
};

#endif
