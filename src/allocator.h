#ifndef UU_ALLOCATOR_H
#define UU_ALLOCATOR_H

#include <stddef.h> /* for size_t */

struct SPDR_Allocator;

extern void *allocator_alloc(struct SPDR_Allocator *allocator, size_t size);
extern void allocator_free(struct SPDR_Allocator *allocator, void *ptr);

#endif
