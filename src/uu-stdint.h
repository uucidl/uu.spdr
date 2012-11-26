#ifndef UU_SPDR_STDINT_H
#define UU_SPDR_STDINT_H

#if defined(_MSC_VER) && _MSC_VER < 1600
/* Before VS 2010, Microsoft did not ship a stdint.h */
#include "../deps/stdint-msvc.h"
#else
#include <stdint.h>
#endif

#endif
