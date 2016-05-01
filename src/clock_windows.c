#include <windows.h>

#include "clock.h"

#include "clock_type.h"

extern int clock_init(struct SPDR_Clock **clockp,
                      struct SPDR_Allocator *allocator)
{
        LARGE_INTEGER qpf;
        if (!QueryPerformanceFrequency(&qpf)) {
                return -1;
        }

        return clock_init_base(clockp, allocator, 1000000,
                               (uint64_t)qpf.QuadPart);
}

extern uint64_t clock_ticks(struct SPDR_Clock const *clock)
{
        LARGE_INTEGER pc;
        QueryPerformanceCounter(&pc);

        (void)clock;

        return (uint64_t)pc.QuadPart;
}
