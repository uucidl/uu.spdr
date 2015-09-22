#ifndef UU_CLOCK_TYPE_H
#define UU_CLOCK_TYPE_H

struct SPDR_Allocator;

struct SPDR_Clock {
        struct SPDR_Allocator *allocator;
        /**
         * numerator/denominator to convert ticks to microseconds
         */
        uint64_t microseconds_per_tick[2];
};

extern int clock_init_base(struct SPDR_Clock **clockp,
                           struct SPDR_Allocator *allocator,
                           uint64_t numerator,
                           uint64_t denominator);

#endif
