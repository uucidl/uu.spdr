#include <mach/mach_time.h>
#include <string.h>

#include "clock.h"

#include "clock_type.h"

extern int clock_init(struct Clock** clockp, struct Allocator* allocator)
{
	mach_timebase_info_data_t info;
	if (mach_timebase_info (&info))
	{
		return -1;
	}

	return clock_init_base(clockp, allocator, info.numer, info.denom * 1000);
}

extern uint64_t clock_ticks(struct Clock const* const clock)
{
	(void) clock;

	return mach_absolute_time();
}
