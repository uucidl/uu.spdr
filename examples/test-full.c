#include <spdr/spdr.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

enum { LOG_N = 2048 * 3 };

static void print(const char *string, void *user_data)
{
        FILE *file = user_data;

        fputs(string, file);
}

static struct SPDR_Context *spdr;

static double take_some_time()
{
        int i;
        double result = 0.3;
        for (i = 0; i < 2048; i++) {
                result += cos(result);
        }
        return result;
}

int main()
{
        struct SPDR_Capacity capacity;
        char *spdr_buffer = malloc(LOG_N);
        if (spdr_init(&spdr, spdr_buffer, LOG_N)) {
                printf("problem initializing SPDR\n");
                return 1;
        }
        spdr_enable_trace(spdr, 1);

        capacity = spdr_capacity(spdr);
        printf("capacity: %ld/%ld\n", capacity.count, capacity.capacity);

        {
                size_t count = capacity.count, previous_count;
                double result = 0.2;
                do {
                        SPDR_BEGIN(spdr, "test", "fill");
                        previous_count = count;
                        capacity = spdr_capacity(spdr);
                        count = capacity.count;
                        result += take_some_time();
                        SPDR_END(spdr, "test", "fill");
                } while (previous_count < count);
                printf("some computation: %f\n", result);
        }

        capacity = spdr_capacity(spdr);
        printf("capacity at the end: %ld/%ld\n", capacity.count,
               capacity.capacity);

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
