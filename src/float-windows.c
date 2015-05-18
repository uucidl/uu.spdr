#include <float.h>

#include "float.h"

extern int float_isfinite(double value) { return _finite(value); }
