#include "chars.h"

#include <stdarg.h>
#include <stdio.h>

#if defined(__clang__)
__attribute__((__format__(__printf__, 2, 3))) spdr_internal void
chars_catsprintf(struct SPDR_Chars *chars, const char *format, ...);
#endif

spdr_internal void
chars_catsprintf(struct SPDR_Chars *chars, const char *format, ...)
{
        if (chars->error) {
                return;
        }

        {
                size_t remaining_capacity = chars->capacity - chars->len;
                int result;
                va_list args;
                va_start(args, format);

                result = vsnprintf(chars->chars + chars->len,
                                   chars->capacity - chars->len, format, args);

                if (result < 0) {
                        chars->error = 1;
                } else if ((size_t)result >= remaining_capacity) {
                        chars->error = 1;
                } else {
                        chars->len += (size_t)result;
                }

                va_end(args);
        }
}
