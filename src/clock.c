#include "allocator.h"
#include "clock.h"
#include "inlines.h"

#include "clock_type.h"

extern int clock_init_base(struct SPDR_Clock **clockp,
                           struct SPDR_Allocator *allocator,
                           uint64_t numerator,
                           uint64_t denominator)
{
        struct SPDR_Clock *clock = SPDR_VOID_PTR_CAST(
            struct SPDR_Clock, allocator_alloc(allocator, sizeof *clock));
        if (!clock) {
                return -1;
        }

        clock->allocator = allocator;
        clock->microseconds_per_tick[0] = numerator;
        clock->microseconds_per_tick[1] = denominator;

        *clockp = clock;

        return 0;
}

extern void clock_deinit(struct SPDR_Clock **clockp)
{
        struct SPDR_Clock *clock = *clockp;
        allocator_free(clock->allocator, clock);
        *clockp = 0;
}

extern uint64_t clock_ticks_to_microseconds(struct SPDR_Clock const *clock,
                                            uint64_t ticks)
{
        return clock->microseconds_per_tick[0] * ticks /
               clock->microseconds_per_tick[1];
}

extern uint64_t clock_microseconds(struct SPDR_Clock const *clock)
{
        return clock_ticks_to_microseconds(clock, clock_ticks(clock));
}
