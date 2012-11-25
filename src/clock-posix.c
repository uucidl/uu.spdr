#include <time.h>

#include "allocator.h"
#include "clock.h"

struct Clock
{
};

extern int clock_init (struct Clock** clockp, struct Allocator* allocator)
{
  (void) allocator;

  *clockp = NULL; 

  return 0;
}

extern void clock_deinit(struct Clock** clockp)
{
	*clockp = NULL;
}

extern uint64_t clock_microseconds(struct Clock* clock)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  return ts.tv_nsec / 1000;
}
