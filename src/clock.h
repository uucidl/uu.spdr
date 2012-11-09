#ifndef UU_CLOCK_H
#define UU_CLOCK_H

struct Clock;

extern int clock_init(struct Clock** clock);
extern void clock_deinit(struct Clock** clock);
extern uint64_t clock_microseconds(struct Clock* clock);

#endif