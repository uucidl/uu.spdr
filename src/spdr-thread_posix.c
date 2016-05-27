#include "spdr-internal.h"

#include <pthread.h>

spdr_internal uint64_t uu_spdr_get_tid(void)
{
        pthread_t x = pthread_self();
        return (uintptr_t)x;
}
