#ifndef UU_SPDR_CHARS_H
#define UU_SPDR_CHARS_H

#include "inlines.h"
#include "uu-stdint.h"

struct SPDR_Chars {
        uint32_t error;
        char *chars;
        size_t len;
        size_t capacity;
};

#define SPDR_Chars_NULL                                                        \
        {                                                                      \
                0, NULL, 0, 0                                                  \
        }

spdr_internal void
chars_catsprintf(struct SPDR_Chars *chars, const char *format, ...);
spdr_internal void chars_catjsonstr(struct SPDR_Chars *chars, const char *utf8);

#endif
