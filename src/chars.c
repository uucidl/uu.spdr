#include "inlines.h"
#include "chars.h"

#include "uu-stdint.h"

/*
*	http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
*/
enum { UTF8_ACCEPT = 0, UTF8_REJECT = 1 };

static const uint8_t utf8d[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   9,   9,   9,   9,   9,   9,
    9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   7,   7,   7,   7,   7,
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
    7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   8,   8,   2,
    2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
    2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0xa,
    0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3,
    0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
    0x8, 0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1,
    0x1, 0x1, 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   0,   1,   1,   1,   1,   1,   0,   1,   0,   1,   1,
    1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   2,   1,   2,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,
    2,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   3,
    1,   3,   1,   1,   1,   1,   1,   1,   1,   3,   1,   1,   1,   1,   1,
    3,   1,   3,   1,   1,   1,   1,   1,   1,   1,   3,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
};

/**
 * decodes UTF-8 streams.
 *
 * @param state must be UTF8_ACCEPT initially
 */
spdr_internal uint32_t decode(uint32_t *state, uint32_t *codep, uint8_t byte)
{
        uint32_t type = utf8d[byte];

        *codep = (*state != UTF8_ACCEPT) ? (byte & 0x3fu) | (*codep << 6)
                                         : (0xff >> type) & (byte);

        *state = utf8d[256 + *state * 16 + type];

        return *state;
}

spdr_internal void chars_catchar(struct SPDR_Chars *chars, const char c)
{
        if (chars->capacity - chars->len < 1) {
                chars->error = 1;
                return;
        }

        chars->chars[chars->len] = c;
        chars->len++;
}

spdr_internal void chars_catjsonstr(struct SPDR_Chars *chars, const char *utf8)
{
        const char *ch;

#define CCODE(c)                                                               \
        {                                                                      \
                chars_catchar(chars, '\\');                                    \
                chars_catchar(chars, c);                                       \
        }                                                                      \
        continue

        uint32_t decoder_state = UTF8_ACCEPT;
        uint32_t code_point = 0;
        for (ch = utf8; ch[0]; ch++) {
                uint8_t byte = (uint8_t)ch[0];
                if (UTF8_ACCEPT == decode(&decoder_state, &code_point, byte)) {
                        if (code_point > 127) {
                                chars_catsprintf(chars, "\\u%04X", code_point);
                        } else {
                                switch (code_point) {
                                case '\a':
                                        CCODE('a');
                                case '\b':
                                        CCODE('b');
                                case '\t':
                                        CCODE('t');
                                case '\n':
                                        CCODE('n');
                                case '\v':
                                        CCODE('v');
                                case '\f':
                                        CCODE('f');
                                case '\r':
                                        CCODE('r');
                                case '\\':
                                case '"':
                                        chars_catchar(chars, '\\');
                                        SPDR_FALLTHROUGH;
                                default:
                                        chars_catchar(chars, ch[0]);
                                }
                        }
                }
        }
#undef CCODE
}
