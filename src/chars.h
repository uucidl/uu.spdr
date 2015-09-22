#ifndef CHARS_H
#define CHARS_H

#include "inlines.h"

struct SPDR_Chars {
        int error;
        char *chars;
        int len;
        int capacity;
};

#define NULL_CHARS                                                             \
        {                                                                      \
                0, NULL, 0, 0                                                  \
        }

spdr_internal void
chars_catsprintf(struct SPDR_Chars *chars, const char *format, ...);
spdr_internal void chars_catjsonstr(struct SPDR_Chars *chars, const char *utf8);

#endif
