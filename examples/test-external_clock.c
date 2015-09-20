#include "sleep.h"

#include <spdr/spdr.h>

#include <timer_lib/timer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

static struct SPDR_Context *spdr;
enum { LOG_N = 2 * 1024 * 1024 };
static void *spdr_buffer;
static timer clock;

void trace(const char *line, void *_)
{
        char buffer[512] = "";
        strncat(buffer, line, sizeof buffer - 2);
        strncat(buffer, "\n", sizeof buffer - 2);

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
        SPDR_BEGIN2(spdr, "Main", "act", SPDR_INT("info-id", (int)a_string),
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

static unsigned long long timer_micros(void *user_data)
{
        timer *timer = user_data;
        tick_t ticks = timer_elapsed_ticks(timer, 0);
        unsigned long long ticks_per_micros =
            timer_ticks_per_second(timer) / 1000000;

        return ticks / ticks_per_micros;
}

int main(int argc, char **argv)
{
        if (timer_lib_initialize() < 0) {
                trace("could not initialize timer library", "hello");
                return;
        }
        timer_initialize(&clock);

        spdr_buffer = malloc(LOG_N);
        spdr_init(&spdr, spdr_buffer, LOG_N);
        spdr_enable_trace(spdr, TRACING_ENABLED);
        spdr_set_log_fn(spdr, trace, NULL);
        spdr_set_clock_microseconds_fn(spdr, timer_micros, &clock);

        SPDR_METADATA1(spdr, "thread_name", SPDR_STR("name", "Main_Thread"));

        SPDR_BEGIN2(spdr, "Main", "main", SPDR_INT("argc", argc),
                    SPDR_STR("argv[0]", argv[0]));

        printf("Hello,");
        sleep(1);
        SPDR_BEGIN1(spdr, "Main", "printf", SPDR_STR("format", " 世界.\n"));
        printf(" 世界.\n");
        SPDR_END(spdr, "Main", "printf");

        stuff();

        SPDR_END(spdr, "Main", "main");

        {
                FILE *file = fopen("trace.json", "wb+");
                if (file) {
                        spdr_report(spdr, SPDR_CHROME_REPORT, print, file);
                        fclose(file);
                }
        }
        spdr_deinit(&spdr);
        free(spdr_buffer);

        timer_lib_shutdown();

        return 0;
}
