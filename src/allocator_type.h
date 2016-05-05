#ifndef UU_SPDR_ALLOCATOR_TYPE_H
#define UU_SPDR_ALLOCATOR_TYPE_H

#include <string.h>

struct SPDR_Allocator {
        void *(*alloc)(struct SPDR_Allocator *self, size_t size);
        void (*free)(struct SPDR_Allocator *self, void *ptr);
};

#endif
