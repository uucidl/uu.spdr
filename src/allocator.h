#ifndef UU_SPDR_ALLOCATOR_H
#define UU_SPDR_ALLOCATOR_H

#include "inlines.h"

#include <stddef.h> /* for size_t */

struct SPDR_Allocator;

spdr_internal void *allocator_alloc(struct SPDR_Allocator *allocator,
                                    size_t size);
spdr_internal void allocator_free(struct SPDR_Allocator *allocator, void *ptr);

#endif
