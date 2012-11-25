#include "allocator_type.h"
#include "allocator.h"

void* allocator_alloc(struct Allocator* allocator, size_t size)
{
	return allocator->alloc(allocator, size);
}

void allocator_free(struct Allocator* allocator, void* ptr)
{
	allocator->free(allocator, ptr);
}

