#include "allocator_type.h"
#include "allocator.h"

void *allocator_alloc(struct SPDR_Allocator *allocator, size_t size)
{
        return allocator->alloc(allocator, size);
}

void allocator_free(struct SPDR_Allocator *allocator, void *ptr)
{
        allocator->free(allocator, ptr);
}
