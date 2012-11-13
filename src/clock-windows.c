#include <windows.h>

#include "clock.h"

struct Clock
{
	void* _;
};

extern int clock_init(struct Clock** clock)
{
	*clock = 0;
	return 0;
}

extern void clock_deinit(struct Clock** clock)
{
}

extern uint64_t clock_microseconds(struct Clock* clock)
{
	DebugBreak(); // Unimplemented
	return 0;
}
