#ifndef CHARS_H
#define CHARS_H

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

extern void chars_catsprintf(struct SPDR_Chars *chars, const char *format, ...);
extern void chars_catjsonstr(struct SPDR_Chars *chars, const char *utf8);

#endif
