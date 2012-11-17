#include <windows.h>

#include "clock.h"
#include "allocator.h"

struct Clock
{
	struct Allocator* allocator;
	LARGE_INTEGER qpf;
	LARGE_INTEGER origin;
};

extern int clock_init(struct Clock** clockp, struct Allocator* allocator)
{
	struct Clock* clock = allocator_alloc(allocator, sizeof *clock);
	if (!clock) {
		return -1;
	}

	clock->allocator = allocator;

	if (!QueryPerformanceFrequency(&clock->qpf)) {
		clock_deinit(clockp);
		return -1;
	}

	QueryPerformanceCounter(&clock->origin);
	
	*clockp = clock;

	return 0;
}

extern void clock_deinit(struct Clock** clockp)
{
	struct Clock* clock = *clockp;
	allocator_free(clock->allocator, clock);
	*clockp = NULL;
}

extern uint64_t clock_microseconds(struct Clock* clock)
{
	uint64_t micros;
	LARGE_INTEGER pc;
	QueryPerformanceCounter(&pc);
	
	micros = ((uint64_t) 1000000) * (pc.QuadPart - clock->origin.QuadPart) / clock->qpf.QuadPart;
		
	return micros;
}
