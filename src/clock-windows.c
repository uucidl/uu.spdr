#include <windows.h>

#include "clock.h"

#include "clock_type.h"

extern int clock_init(struct Clock** clockp, struct Allocator* allocator)
{
	LARGE_INTEGER qpf;
	if (!QueryPerformanceFrequency(&qpf)) {
		return -1;
	}

	return clock_init_base (clockp, allocator, 1000000, qpf.QuadPart);
}

extern uint64_t clock_ticks(struct Clock const * clock)
{
	LARGE_INTEGER pc;
	QueryPerformanceCounter(&pc);

	(void) clock;

	return pc.QuadPart;
}

