#include <math.h>

#include "float.h"

/* posix.1 or c99 provide isfinite */

spdr_internal int float_isfinite(double value) { return isfinite(value); }
