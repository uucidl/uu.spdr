#include "chars.h"

void chars_catchar(struct Chars* chars, const char c)
{
	if (chars->capacity - chars->len < 1) {
		chars->error = 1;
		return;
	}

	chars->chars[chars->len] = c;
	chars->len++;
}

void chars_catjsonstr(struct Chars* chars, const char* utf8)
{
	const char* ch;

#define CCODE(c) do { \
	chars_catchar(chars, '\\'); \
	chars_catchar(chars, c); \
	} while (0); \
	continue

	for (ch = utf8; ch[0]; ch++) {
		switch(ch[0]) {
		case '\a': CCODE('a');
		case '\b': CCODE('b');
		case '\t': CCODE('t');
		case '\n': CCODE('n');
		case '\v': CCODE('v');
		case '\f': CCODE('f');
		case '\r': CCODE('r');
		case '\\':
		case '"':
			chars_catchar(chars, '\\');
		default:
			chars_catchar(chars, ch[0]);
		}
	}
#undef CCODE
}
