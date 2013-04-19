#include <time.h>

#include "clock.h"

#include "clock_type.h"

extern int clock_init (struct Clock** clockp, struct Allocator* allocator)
{
	struct timespec res = { 0 };

	if (clock_getres(CLOCK_MONOTONIC, &res) != 0 ||
	    res.tv_nsec > 1000) {
		/* unknown clock or insufficient resolution */
		return -1;
	}

	return clock_init_base(clockp, allocator, 1, 1000);
}

extern uint64_t clock_ticks(struct Clock const * const clock)
{
	struct timespec ts = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &ts);

	(void) clock;

	return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}
