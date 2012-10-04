#include "spdr-internal.h"

#include <windows.h>

uint32_t uu_spdr_get_pid()
{
	return GetCurrentProcessId();
}

uint64_t uu_spdr_get_tid()
{
	return GetCurrentThreadId();
}
