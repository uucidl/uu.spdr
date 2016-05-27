#include "spdr-internal.h"

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

uint32_t uu_spdr_get_pid(void) { return (uint32_t)getpid(); }
