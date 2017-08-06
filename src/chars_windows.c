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
                int count;
                va_list args;
                va_start(args, format);

                count =
                    vsnprintf(chars->chars + chars->len,
                              chars->capacity - chars->len - 1, format, args);

                if (count < 0) {
                        chars->error = 1;
                } else {
                        chars->len += (size_t)count;
                }

                va_end(args);
        }
}
