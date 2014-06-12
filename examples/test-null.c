#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#include <spdr/spdr.h>

#include "sleep.h"

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

static struct SPDR_Context* spdr;

void trace (const char* line, void* _)
{
        char buffer[512] = "";
        strncat(buffer, line, sizeof buffer - 2);
        strncat(buffer, "\n", sizeof buffer - 2);

        (void) _;

        /* fputs is thread-safe */
        fputs (buffer, stdout);
}

void print (const char* string, void* user_data)
{
        FILE* file = user_data;

        fputs (string, file);
}

int main (int argc, char** argv)
{
        spdr_init_null(&spdr);
        spdr_enable_trace(spdr, TRACING_ENABLED);
        spdr_set_log_fn(spdr, trace, NULL);

        SPDR_METADATA1(spdr, "thread_name", SPDR_STR("name", "Main_Thread"));

        SPDR_BEGIN2(spdr, "Main", "main",
                    SPDR_INT("argc", argc),
                    SPDR_STR("argv[0]", argv[0]));

        printf ("Hello,");
        sleep (1);
        SPDR_BEGIN1(spdr, "Main", "printf", SPDR_STR("format", " 世界.\n"));
        printf (" 世界.\n");
        SPDR_END(spdr, "Main", "printf");

        SPDR_END(spdr, "Main", "main");

        spdr_deinit(&spdr);

        return 0;
}
