#include <spdr/spdr.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

enum { LOG_N = 2 * 1024 * 1024 };
static void *spdr_buffer;
static struct SPDR_Context *spdr;

static void trace(const char *line, void *user_data)
{
        const char *msg = user_data;
        char buffer[512] = "";

        assert(0 == strcmp(msg, "Hello"));

        strncat(buffer, line, sizeof buffer - 2);
        strncat(buffer, "\n", sizeof buffer - 2);

        /* this is ok because fputs is thread-safe */
        fputs(buffer, stderr);
}

static void fun1()
{
        static double x = 0.5;
        static double y = -0.15;

        SPDR_SCOPE2(spdr, "Main", "fun1", SPDR_FLOAT("x", x),
                    SPDR_FLOAT("y", y));

        int N = 65536;
        while (N--) {
                y = cos(x + atan2(x, y));
                x = sin(y);
        }
}

int main(int argc, char **argv)
{
        spdr_buffer = malloc(LOG_N);
        spdr_init(&spdr, spdr_buffer, LOG_N);
        spdr_enable_trace(spdr, TRACING_ENABLED);
        spdr_set_log_fn(spdr, trace, "Hello");

        SPDR_BEGIN2(spdr, "Main", "main", SPDR_INT("argc", argc),
                    SPDR_STR("argv[0]", argv[0]));

        printf("Hello,");
        sleep(3);
        printf(" 世界.\n");

        fun1();
        fun1();
        fun1();

        SPDR_END(spdr, "Main", "main");

        spdr_deinit(&spdr);
        free(spdr_buffer);

        return 0;
}
