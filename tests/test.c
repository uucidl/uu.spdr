#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "spdr.h"
#include "sleep.h"

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

static struct spdr* spdr;
enum { LOG_N = 2 * 1024 * 1024 };
static void* spdr_buffer;

void trace (const char* line, void* _)
{
	char buffer[512] = "";
	strncat(buffer, line, sizeof buffer - 2);
	strncat(buffer, "\n", sizeof buffer - 2);

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
	spdr_buffer = malloc(LOG_N);
	spdr_init(&spdr, spdr_buffer, LOG_N);
	spdr_enable_trace(spdr, TRACING_ENABLED);
	spdr_set_log_fn(spdr, trace, NULL);

	SPDR_METADATA1(spdr, "thread_name", SPDR_STR("name", "Main_Thread"));

	SPDR_BEGIN2(spdr, "Main", "main",
		    SPDR_INT("argc", argc),
			SPDR_STR("argv[0]", argv[0]));

	printf ("Hello,");
	sleep (3);
	printf (" 世界.\n");

	SPDR_END(spdr, "Main", "main");

	{
		FILE* file = fopen("trace.json", "wb+");
		if (file) {
			spdr_report(spdr, SPDR_CHROME_REPORT, print, file);
			fclose(file);
		}
	}
	spdr_deinit(&spdr);
	free(spdr_buffer);

	return 0;
}
