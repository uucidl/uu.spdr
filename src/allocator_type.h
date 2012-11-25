#ifndef UU_ALLOCATOR_TYPE_H
#define UU_ALLOCATOR_TYPE_H

#include <string.h>

struct Allocator
{
	void* (*alloc)(struct Allocator* self, size_t size);
	void  (*free)(struct Allocator* self, void* ptr);
};

#endif