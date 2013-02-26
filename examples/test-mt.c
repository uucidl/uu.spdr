#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "spdr.h"

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

static struct spdr* spdr;
enum { LOG_N = 6 * 1024 };
static void* spdr_buffer;

void trace (const char* line, void* user_data)
{
	char buffer[512] = "";
	const char* msg = user_data;

	assert(0 == strcmp(msg, "Hello"));

	strncat(buffer, line, sizeof buffer - 2);
	strncat(buffer, "\n", sizeof buffer - 2);

	/* fputs is thread-safe */
	fputs (buffer, stderr);
}

void print (const char* str, void* user_data)
{
	const char* msg = user_data;
	assert(0 == strcmp(msg, "Hello"));

	fputs(str, stderr);
}

void* thread1(void* arg)
{
	int n = 30;
	double x = 0.1234;
	double y = 0.117;

	SPDR_METADATA1(spdr, "thread_name", SPDR_STR("name", "Thread1"));

	while (n--) {
		int cosn = 16*65536;
		int pown = 32*65536;

		SPDR_BEGIN2(spdr, "Main", "thread1", SPDR_INT("arg", (intptr_t) arg), SPDR_FLOAT("y", y));
		while (cosn--) {
			x += cos(x);
		}
		SPDR_END(spdr, "Main", "thread1");

		while (pown--) {
			y += atan2(y, x);
		}
	}

	pthread_exit(NULL);
	return NULL;
}

int main (int argc, char** argv)
{
	pthread_t thread;
	struct spdr_capacity cap;

	spdr_buffer = malloc(LOG_N);
	spdr_init(&spdr, spdr_buffer, LOG_N);

	cap = spdr_capacity(spdr);
	printf ("spdr capacity: %ld/%ld\n", cap.count, cap.capacity);

	spdr_enable_trace(spdr, TRACING_ENABLED);

	SPDR_METADATA1(spdr, "thread_name", SPDR_STR("name", "Main_Thread"));

	SPDR_BEGIN2(spdr, "Main", "main",
		    SPDR_INT("argc", argc), SPDR_STR("argv[0]", argv[0]));

	pthread_create(&thread, NULL, thread1, NULL);

	printf ("Hello,");
	sleep (3);
	printf (" 世界.\n");

	SPDR_BEGIN(spdr, "Main", "Waiting For Thread1");
	{
		void* status;
		pthread_join (thread, &status);
	}
	SPDR_END(spdr, "Main", "Waiting For Thread1");

	SPDR_END(spdr, "Main", "main");

	cap = spdr_capacity(spdr);
	printf ("spdr capacity: %ld/%ld\n", cap.count, cap.capacity);
	spdr_report(spdr, SPDR_CHROME_REPORT, print, "Hello");
	spdr_deinit(&spdr);
	free(spdr_buffer);

	pthread_exit (NULL);

	return 0;
}
