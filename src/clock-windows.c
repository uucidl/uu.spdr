#include <windows.h>

#include "clock.h"
#include "allocator.h"

struct Clock
{
	struct Allocator* allocator;
};

extern int clock_init(struct Clock** clockp, struct Allocator* allocator)
{
	struct Clock* clock = allocator_alloc(allocator, sizeof *clock);
	if (!clock) {
		return -1;
	}

	clock->allocator = allocator;

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
	DebugBreak(); // Unimplemented
	return 0;
}
