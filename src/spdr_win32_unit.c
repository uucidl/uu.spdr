#ifdef __cplusplus
extern "C" {
#endif

#define _CRT_SECURE_NO_WARNINGS // we use vsnprintf carefully.

#include "allocator.c"
#include "chars.c"
#include "chars_windows.c"
#include "clock.c"
#include "clock_windows.c"
#include "float_windows.c"
#include "spdr.c"
#include "spdr_windows.c"

#undef _CRT_SECURE_NO_WARNINGS

#ifdef __cplusplus
}
#endif
