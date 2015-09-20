#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#include <spdr/spdr.h>

#include "sleep.h"

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

static struct SPDR_Context *spdr;
enum { LOG_N = 2 * 1024 * 1024 };
static void *spdr_buffer;

void trace(const char *line, void *_)
{
        char buffer[512] = "";
        strncat(buffer, line, sizeof buffer - 2);
        strncat(buffer, "\n", sizeof buffer - 2);

        (void)_;

        /* fputs is thread-safe */
        fputs(buffer, stdout);
}

void print(const char *string, void *user_data)
{
        FILE *file = user_data;

        fputs(string, file);
}

static void act(const char *a_string)
{
        SPDR_BEGIN2(spdr, "Main", "act", SPDR_INT("info-id", (intptr_t)a_string),
                    SPDR_STR("info", a_string));

        printf("%s\n", a_string);
        sleep(1);

        SPDR_END(spdr, "Main", "act");
}

static void stuff()
{
        {
                char str[64] = "A Dynamically Allocated String";
                act(str);
        }
        {
                char str[64] = "Another Dynamically Allocated String";
                act(str);
        }
}

int main(int argc, char **argv)
{
        spdr_buffer = malloc(LOG_N);
        spdr_init(&spdr, spdr_buffer, LOG_N);
        spdr_enable_trace(spdr, TRACING_ENABLED);
        spdr_set_log_fn(spdr, trace, NULL);

        SPDR_METADATA1(spdr, "thread_name", SPDR_STR("name", "Main_Thread"));

        SPDR_BEGIN2(spdr, "Main", "main", SPDR_INT("argc", argc),
                    SPDR_STR("argv[0]", argv[0]));

        printf("Hello,");
        sleep(1);
        SPDR_BEGIN1(spdr, "Main", "printf", SPDR_STR("format", " 世界.\n"));
        printf(" 世界.\n");
        SPDR_END(spdr, "Main", "printf");

        stuff();

        {
                double a = 0.0;
                SPDR_EVENT2(spdr, "Main", "Non Finites",
                            SPDR_FLOAT("a", 1.0 / a),
                            SPDR_FLOAT("b", sqrt(-1)));
        }

        SPDR_END(spdr, "Main", "main");

        {
                int i;
                for (i = 0; i < 100; i++) {
                        SPDR_COUNTER2(spdr, "Main", "counter", SPDR_INT("i", i),
                                      SPDR_FLOAT("cos(i)", cos(3.141592 * i / 50)));
                }
        }

        {
                FILE *file = fopen("trace.json", "wb+");
                if (file) {
                        spdr_report(spdr, SPDR_CHROME_REPORT, print, file);
                        fclose(file);
                }
        }
        spdr_deinit(&spdr);
        free(spdr_buffer);

        return 0;
}
