#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "spdr.h"


#include "../src/clock.h"
#include "../src/allocator_type.h"

#ifndef TRACING_ENABLED
#define TRACING_ENABLED 0
#endif

static struct SPDR_Context* gbl_spdr;

static void* std_allocator_alloc(struct Allocator* self, size_t size)
{
	(void) self;

	return malloc(size);
}

static void std_allocator_free(struct Allocator* self, void* ptr)
{
	(void) self;

	free(ptr);
}

struct ThreadContext
{
	volatile double* results;
	int offset;
	int n;

	struct Clock* clock;
	uint64_t ts0;
	uint64_t ts1;

	char buffer[1024];
	int terminated_p;
	pthread_t pthread;
};

static void* thread_main (void* ctxt)
{
	struct ThreadContext* context = ctxt;
	int const chunks_n = 1024;
	int i;
	int j = 0;
	struct SPDR_Context* spdr = gbl_spdr;

	context->ts0 = clock_microseconds(context->clock);

	SPDR_BEGIN1(spdr, "thread_main", "main",
		    SPDR_INT("i", context->offset));

	while (j < context->n) {
		SPDR_BEGIN1(spdr, "main", "::sin", SPDR_INT("i", j));
		for (i = 0; i < chunks_n; i++) {
			if (j + i >= context->n) {
				break;
			}

			context->results[j + i] = sin(context->results[j + i]);
		}
		SPDR_END(spdr, "main", "::sin");

		j = j + chunks_n;
	}

	SPDR_END(spdr, "thread_main", "main");

	context->ts1 = clock_microseconds(context->clock);

	return NULL;
}

static void report(const char* str, void* user_data)
{
	fputs(str, (FILE*) user_data);
}

extern int main(int argc, char** argv)
{
	enum { LOG_N = 256 * 1024 * 1024 };
	void* spdr_buffer = malloc(LOG_N);
	uint64_t single_threaded_ms = 0;
	struct Clock* clock;
	struct Allocator std_allocator = { std_allocator_alloc, std_allocator_free };
	clock_init(&clock, &std_allocator);

	spdr_init(&gbl_spdr, spdr_buffer, LOG_N);
	spdr_enable_trace(gbl_spdr, TRACING_ENABLED);

	SPDR_METADATA1(gbl_spdr, "thread_name", SPDR_STR("name", "Main_Thread"));
	{
		int N = 10000;
		uint64_t ts0 = clock_microseconds(clock);

		SPDR_BEGIN(gbl_spdr, "main", "single_threaded_test");

		while (N--) {
			int IN = 64 * 1024;
			volatile double results[IN];

			SPDR_BEGIN1(gbl_spdr, "main", "::sin", SPDR_INT("i", IN));
			while (IN--) {
				results[IN] = sin(results[IN]);
			}
			SPDR_END(gbl_spdr, "main", "::sin");
		}

		single_threaded_ms = (clock_microseconds(clock) - ts0) / 1000;
		printf("elapsed_ms: %llu\n", single_threaded_ms);
		SPDR_END(gbl_spdr, "main", "single_threaded_test");
	}

	{
		int N = 10;
		SPDR_BEGIN(gbl_spdr, "main", "multi_threaded_test");

		while (N--) {
			int const IN_TN = 8;
			int IN = 8 * 100 * 64 * 1024;
			double* results = malloc(IN * sizeof(*results));

			struct ThreadContext threads[IN_TN];

			{
				int i;
				int offset = 0;
				int const n = IN / IN_TN;

				for (i = 0; i < IN_TN; i++) {
					threads[i].clock        = clock;
					threads[i].results      = &results[offset];
					threads[i].offset       = offset;
					threads[i].n            = n;
					threads[i].terminated_p = 0;

					pthread_create(&threads[i].pthread, NULL, thread_main, &threads[i]);

					offset += n;
				}
			}

			{
				uint64_t earlier_start = clock_microseconds(clock);
				uint64_t later_end = earlier_start;
				int i;

				for (i = 0; i < IN_TN; i++) {
					pthread_join (threads[i].pthread, NULL);

					threads[i].terminated_p = 0;
					if (threads[i].ts0 < earlier_start) {
						earlier_start = threads[i].ts0;
					}

					if (threads[i].ts1 > later_end) {
						later_end = threads[i].ts1;
					}
				}

				{
					uint64_t elapsed_ms = (later_end - earlier_start) / 1000;
					uint64_t equivalent_ms = 10 * elapsed_ms;
					double scaling = (double) single_threaded_ms / equivalent_ms;
					printf("elapsed_ms: %llu\n", elapsed_ms);
					printf("equivalent in ms: %llu (scaling: %f)\n", equivalent_ms, scaling);
				}
			}

			free(results);
		}
		SPDR_END(gbl_spdr, "main", "multi_threaded_test");
	}

	{
		FILE* f = fopen("perf.json", "wb");
		if (f) {
			spdr_report(gbl_spdr, SPDR_CHROME_REPORT, report, f);
			fclose(f);
		}
	}

	clock_deinit(&clock);
	spdr_deinit(&gbl_spdr);
	free(spdr_buffer);
	spdr_buffer = NULL;

	return 0;
}
