#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "spdr.h"

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

static struct spdr* spdr;

void trace (const char* line)
{
	char buffer[512] = "";
	strncat(buffer, line, sizeof buffer - 2);
	strncat(buffer, "\n", sizeof buffer - 2);

	// fputs is thread-safe
	fputs (buffer, stderr);
}

void* thread1(void* arg)
{
	SPDR_BEGIN(spdr, "Main", "thread1");

	SPDR_END(spdr, "Main", "thread1");

	return NULL;
}

int main (int argc, char** argv)
{
	spdr_init(&spdr);
	spdr_enable_trace(spdr, TRACING_ENABLED);
	spdr_set_log_fn(spdr, trace);

	SPDR_BEGIN(spdr, "Main", "main");
	pthread_t thread;
	pthread_create(&thread, NULL, thread1, NULL);

	printf ("Hello,");
	sleep (3);
	printf (" 世界.\n");

	void* status;
	pthread_join (thread, status);

	SPDR_END(spdr, "Main", "main");
	pthread_exit (NULL);
}
