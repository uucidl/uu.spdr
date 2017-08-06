#ifdef __cplusplus
extern "C" {
#endif

#if _POSIX_C_SOURCE < 200112L
#define _POSIX_C_SOURCE 200112L
#endif

/* generic modern POSIX platforms */

#include "allocator.c"
#include "chars.c"
#include "chars_posix.c"
#include "clock.c"
#include "clock_posix.c"
#include "float_posix.c"
#include "spdr-thread_posix.c"
#include "spdr.c"
#include "spdr_posix.c"

#undef _POSIX_C_SOURCE

#ifdef __cplusplus
}
#endif
