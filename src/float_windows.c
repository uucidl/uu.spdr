#include <float.h>

#include "float.h"

spdr_internal int float_isfinite(double value) { return _finite(value); }
