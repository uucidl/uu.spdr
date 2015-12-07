#include "chars.h"

#include <stdarg.h>
#include <stdio.h>

spdr_internal void
chars_catsprintf(struct SPDR_Chars *chars, const char *format, ...)
{
        if (chars->error) {
                return;
        }

        {
                int remaining_capacity = chars->capacity - chars->len;
                int count;
                va_list args;
                va_start(args, format);

                count = vsnprintf(chars->chars + chars->len,
                                  chars->capacity - chars->len,
                                  format,
                                  args);

                if (count < 0 || count >= remaining_capacity) {
                        chars->error = 1;
                } else {
                        chars->len += count;
                }

                va_end(args);
        }
}
