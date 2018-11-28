#include <time.h>

#include "clock.h"

#include "clock_type.h"

extern int clock_init(struct SPDR_Clock **clockp,
                      struct SPDR_Allocator *allocator)
{
        struct timespec res;

        if (clock_getres(CLOCK_MONOTONIC, &res) != 0 || res.tv_nsec > 1000) {
                /* unknown clock or insufficient resolution */
                return -1;
        }

        return clock_init_base(clockp, allocator, 1, 1000);
}

extern uint64_t clock_ticks(struct SPDR_Clock const *const clock)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        (void)clock;

        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}
