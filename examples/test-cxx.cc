#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "sleep.h"

#include <spdr/spdr.hh>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

static struct SPDR_Context *spdr;
enum { LOG_N = 2 * 1024 * 1024 };

static void trace(const char *line, void *)
{
        char buffer[512] = "";
        strncat(buffer, line, sizeof buffer - 2);
        strncat(buffer, "\n", sizeof buffer - 2);

        /* fputs is thread-safe */
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
        std::vector<char> buffer(LOG_N);
        spdr_init(&spdr, &buffer.front(), LOG_N);
        spdr_enable_trace(spdr, TRACING_ENABLED);
        spdr_set_log_fn(spdr, trace, nullptr);

        SPDR_METADATA1(spdr, "thread_name", SPDR_STR("name", "Main_Thread"));

        SPDR_BEGIN2(spdr, "Main", "main", SPDR_INT("argc", argc),
                    SPDR_STR("argv[0]", argv[0]));

        printf("Hello,");
        sleep(3);
        printf(" 世界.\n");

        fun1();

        SPDR_END(spdr, "Main", "main");
        {
                FILE *file = fopen("test-cxx.json", "wb+");
                if (file) {
                        spdr_report(
                            spdr, SPDR_CHROME_REPORT,
                            [](const char *data, void *user_data) {
                                    fputs(data, static_cast<FILE *>(user_data));
                            },
                            file);
                        fclose(file);
                }
        }
        spdr_deinit(&spdr);

        return 0;
}
