#include "spdr-internal.h"

#include <pthread.h>

uint64_t uu_spdr_get_tid()
{
	return (intptr_t) pthread_self();
}
