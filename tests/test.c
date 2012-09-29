#include <stdio.h>
#include <string.h>

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

int main (int argc, char** argv)
{
	spdr_init(&spdr);
	spdr_enable_trace(spdr, TRACING_ENABLED);
	spdr_set_log_fn(spdr, trace);

	SPDR_BEGIN(spdr, "Main", "main");

	printf ("Hello,");
	sleep (3);
	printf (" 世界.\n");

	SPDR_END(spdr, "Main", "main");
}
