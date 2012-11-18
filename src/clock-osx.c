#include <mach/mach_time.h>
#include <string.h>

#include "allocator.h"

struct Clock
{
	struct Allocator* allocator;
	struct mach_timebase_info timebase_info;
};

extern int clock_init(struct Clock** clockp, struct Allocator* allocator)
{
	mach_timebase_info_data_t info;
	if (mach_timebase_info (&info))
	{
		return -1;
	}

	struct Clock* clock = allocator_alloc(allocator, sizeof *clock);
	if (!clock)
	{
		return -1;
	}

	clock->allocator = allocator;
	clock->timebase_info = info;

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
	uint64_t micros = mach_absolute_time() *
		clock->timebase_info.numer /
		clock->timebase_info.denom / 1000;

	return micros;
}
