#ifndef UU_SPDR_CLOCK_H
#define UU_SPDR_CLOCK_H

#include "inlines.h"
#include "uu-stdint.h" /* uint64_t */

struct SPDR_Clock;
struct SPDR_Allocator;

extern int clock_init_base(struct SPDR_Clock **clockp,
                           struct SPDR_Allocator *allocator,
                           uint64_t numerator,
                           uint64_t denominator);

extern int clock_init(struct SPDR_Clock **clock,
                      struct SPDR_Allocator *allocator);
extern void clock_deinit(struct SPDR_Clock **clock);
extern uint64_t clock_ticks(struct SPDR_Clock const *clock);
extern uint64_t clock_ticks_to_microseconds(struct SPDR_Clock const *clock,
                                            uint64_t ticks);
extern uint64_t clock_microseconds(struct SPDR_Clock const *clock);

#endif
