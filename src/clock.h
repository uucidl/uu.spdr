#ifndef UU_CLOCK_H
#define UU_CLOCK_H

#include "uu-stdint.h" // uint64_t

struct Clock;
struct Allocator;

extern int clock_init(struct Clock** clock, struct Allocator* allocator);
extern void clock_deinit(struct Clock** clock);
extern uint64_t clock_microseconds(struct Clock* clock);

#endif