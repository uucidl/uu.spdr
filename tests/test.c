#include <stdio.h>

#include <spdr.h>

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

static struct spdr* spdr;

int main (int argc, char** argv)
{
	spdr_init(&spdr);
	spdr_enable_trace(spdr, TRACING_ENABLED);

	SPDR_BEGIN(spdr, "Main", "main");

	printf ("Hello, World\n");

	SPDR_END(spdr, "Main", "main");
}
