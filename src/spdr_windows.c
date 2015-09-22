#include "spdr-internal.h"

#include <windows.h>

spdr_internal uint32_t uu_spdr_get_pid() { return GetCurrentProcessId(); }

spdr_internal uint64_t uu_spdr_get_tid() { return GetCurrentThreadId(); }
