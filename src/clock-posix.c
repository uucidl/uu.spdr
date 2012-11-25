#include <time.h>

#include "allocator.h"
#include "clock.h"

struct Clock
{
};

extern int clock_init (struct Clock** clockp, struct Allocator* allocator)
{
  struct timespec res = { 0 };

  (void) allocator;
  
  if (clock_getres(CLOCK_MONOTONIC, &res) != 0 || 
      res.tv_nsec > 1000) {
    /* unknown clock or insufficient resolution */
    return -1;
  }

  *clockp = NULL; 

  return 0;
}

extern void clock_deinit(struct Clock** clockp)
{
	*clockp = NULL;
}

extern uint64_t clock_microseconds(struct Clock* clock)
{
  struct timespec ts = { 0 };
  clock_gettime(CLOCK_MONOTONIC, &ts);

  return ts.tv_nsec / 1000;
}
