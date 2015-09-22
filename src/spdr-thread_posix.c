#include "spdr-internal.h"

#include <pthread.h>

spdr_internal uint64_t uu_spdr_get_tid() { return (intptr_t)pthread_self(); }
