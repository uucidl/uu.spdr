#ifndef UU_SPDR_CLOCK_TYPE_H
#define UU_SPDR_CLOCK_TYPE_H

struct SPDR_Allocator;

struct SPDR_Clock {
        struct SPDR_Allocator *allocator;
        /**
         * numerator/denominator to convert ticks to microseconds
         */
        uint64_t microseconds_per_tick[2];
};

#endif
