#ifndef UU_SPDR_H
#define UU_SPDR_H

//with this define the complete profiler can be disabled
//#define SPDR_PROFILING_DISABLED

/**
 * A tracing library
 */

#include "spdr-private.h"

#include <stddef.h> /* for size_t */

/**
 * Context for the library
 */
struct spdr_context_context;

/**
 * Buffer capacity
 */
struct spdr_capacity
{
	size_t count;
	size_t capacity;
};

/**
 * Reporting type.
 */
enum spdr_report_type
{
	SPDR_PLAIN_REPORT,
	SPDR_CHROME_REPORT,
};

#ifdef SPDR_PROFILING_DISABLED
	#define SPDR_FUNC(x) x{}
	#define SPDR_FUNC_RET(x) x{return -1;}
#else
	#define SPDR_FUNC(x) x
	#define SPDR_FUNC_RET(x) x
#endif

/**
 * Initializes the library
 *
 * spdr will use the provided memory buffer for its memory
 * allocations.
 *
 * the size of the memory buffer will limit the number of events that
 * can be recorded at a time.
 *
 * @return 0 on success
 */
SPDR_FUNC_RET(int spdr_init(struct spdr_context **context, void* buffer, size_t buffer_size));

/**
 * Initializes the library
 *
 * but you provide informations about memory and buffer is allocated by the library itself
 * this is easier to handle when profiling is disabled
 *
 * @return 0 on success
 */
SPDR_FUNC_RET(int spdr_init_allocbuffer(struct spdr_context **context, size_t buffer_size));

/**
 * Shutdowns the library
 */
SPDR_FUNC(void spdr_deinit(struct spdr_context** context));

/**
 * Activates the recording of traces (off by default)
 */
SPDR_FUNC(void spdr_enable_trace(struct spdr_context *context, int traceon));

/**
 * Get status if profiling is available(compiled in) or not (can be used to not write files if not active)
 */
inline int spdr_is_available() { 
#ifdef SPDR_PROFILING_DISABLED
	return 0;
#else
	return 1;
#endif
}

/**
 * Clears the log buffer to start anew
 */
SPDR_FUNC(void spdr_reset(struct spdr_context* context));

/**
 * Returns the current event count and total available capacity
 */
#ifndef SPDR_PROFILING_DISABLED
	struct spdr_capacity spdr_capacity(struct spdr_context* context);
#else
	struct spdr_capacity spdr_capacity(struct spdr_context* context) {
		struct spdr_capacity ret = {0};
		return ret;
	}
#endif

/**
 * Provide your own clock function.
 *
 * It must return a strictly monotonic series of numbers
 * representing elapsed microseconds.
 *
 * It must be thread-safe: it will be called concurrently
 * from multiple threads
 */
SPDR_FUNC(void spdr_set_clock_microseconds_fn(struct spdr_context *context,
		     unsigned long long (*clock_microseconds_fn)(void* user_data),
		     void *user_data));


/**
 * Provide your logging function if you want a trace stream to be produced.
 */
SPDR_FUNC(void spdr_set_log_fn(struct spdr_context *context,
		     void (*log_fn) (const char* line, void* user_data),
		     void *user_data));

/**
 * Report the traces which have been recorded so far, using the
 * provided log function.
 */
SPDR_FUNC(void spdr_report(struct spdr_context *context,
		 enum spdr_report_type report_type,
		 void (*print_fn) (const char* string, void* user_data),
		 void* user_data));


/**
 * Builds arguments of various types
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_INT(key, value)   UU_SPDR_INT(key, value)
	#define SPDR_FLOAT(key, value) UU_SPDR_FLOAT(key, value)
	#define SPDR_STR(key, value)   UU_SPDR_STR(key, value)
#else
	#define SPDR_INT(key, value)
	#define SPDR_FLOAT(key, value)
	#define SPDR_STR(key, value)
#endif

/* __ Instant events __ */

/**
 * An instant event
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_EVENT(spdr, cat, name)			\
		UU_SPDR_TRACE(spdr, cat, name, SPDR_EVENT)
#else
	#define SPDR_EVENT(spdr, cat, name)
#endif


/**
 * An instant event with one parameter
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_EVENT1(spdr, cat, name, arg0)			\
		UU_SPDR_TRACE1(spdr, cat, name, SPDR_EVENT, arg0)
#else
	#define SPDR_EVENT1(spdr, cat, name, arg0)
#endif

/**
 * An instant event with two parameters
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_EVENT2(spdr, cat, name, arg0, arg1)		\
		UU_SPDR_TRACE2(spdr, cat, name, SPDR_EVENT, arg0, arg1)
#else
	#define SPDR_EVENT2(spdr, cat, name, arg0, arg1)
#endif

/**
 * An instant event with two parameters
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_EVENT3(spdr, cat, name, arg0, arg1, arg2)		\
		UU_SPDR_TRACE3(spdr, cat, name, SPDR_EVENT, arg0, arg1, arg2)
#else
	#define SPDR_EVENT3(spdr, cat, name, arg0, arg1, arg2)
#endif


/* __ Work slices __ */

/**
 * Begin a slice of work
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_BEGIN(spdr, cat, name)			\
		UU_SPDR_TRACE(spdr, cat, name, SPDR_BEGIN)
#else
	#define SPDR_BEGIN(spdr, cat, name)
#endif

/**
 * Begin a slice of work, with one parameter.
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_BEGIN1(spdr, cat, name, arg0)			\
		UU_SPDR_TRACE1(spdr, cat, name, SPDR_BEGIN, arg0)
#else
	#define SPDR_BEGIN1(spdr, cat, name, arg0)
#endif

/**
 * Begin a slice of work, with two parameters
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_BEGIN2(spdr, cat, name, arg0, arg1)		\
		UU_SPDR_TRACE2(spdr, cat, name, SPDR_BEGIN, arg0, arg1)
#else
	#define SPDR_BEGIN2(spdr, cat, name, arg0, arg1)
#endif

/**
 * Begin a slice of work, with three parameters
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_BEGIN3(spdr, cat, name, arg0, arg1, arg2)			\
		UU_SPDR_TRACE3(spdr, cat, name, SPDR_BEGIN, arg0, arg1, arg2)
#else
	#define SPDR_BEGIN3(spdr, cat, name, arg0, arg1, arg2)
#endif

/**
 * End a slice of work
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_END(spdr, cat, name)			\
		UU_SPDR_TRACE(spdr, cat, name, SPDR_END)
#else
	#define SPDR_END(spdr, cat, name)
#endif

/**
 * Mark the beginning and end of a scope
 *
 * Non standard. Only supported on select compilers.
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_SCOPE(spdr, cat, name)		\
		UU_SPDR_SCOPE_TRACE(spdr, cat, name)
#else
	#define SPDR_SCOPE(spdr, cat, name)
#endif

#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_SCOPE1(spdr, cat, name, arg0)		\
		UU_SPDR_SCOPE_TRACE1(spdr, cat, name, arg0)
#else
	#define SPDR_SCOPE1(spdr, cat, name, arg0)
#endif

#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_SCOPE2(spdr, cat, name, arg0, arg1)		\
		UU_SPDR_SCOPE_TRACE2(spdr, cat, name, arg0, arg1)
#else
	#define SPDR_SCOPE2(spdr, cat, name, arg0, arg1)
#endif

#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_SCOPE3(spdr, cat, name, arg0, arg1, arg2)		\
		UU_SPDR_SCOPE_TRACE3(spdr, cat, name, arg0, arg1, arg2)
#else
	#define SPDR_SCOPE3(spdr, cat, name, arg0, arg1, arg2)
#endif


/* __ Counters __ */

/**
 * Track values over time.
 *
 * @param arg must be SPDR_INT or SPDR_FLOAT
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_COUNTER1(spdr, cat, name, arg0)	\
		UU_SPDR_TRACE1(spdr, cat, name, SPDR_COUNTER, arg0)
#else
	#define SPDR_COUNTER1(spdr, cat, name, arg0)
#endif

#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_COUNTER2(spdr, cat, name, arg0, arg1)			\
		UU_SPDR_TRACE2(spdr, cat, name, SPDR_COUNTER, arg0, arg1)
#else
	#define SPDR_COUNTER2(spdr, cat, name, arg0, arg1)
#endif

#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_COUNTER3(spdr, cat, name, arg0, arg1, arg2)			\
		UU_SPDR_TRACE3(spdr, cat, name, SPDR_COUNTER, arg0, arg1, arg2)
#else
	#define SPDR_COUNTER3(spdr, cat, name, arg0, arg1, arg2)
#endif

/* __ Metadata __ */

/**
 * Metadata.
 *
 * For instance to set the thread name:
 * @code
 * SPDR_METADATA1("thread_name", SPDR_STR("name", "My_Thread"))
 * @code
 *
 * chrome://tracing will then display this name rather than the tid of
 * the thread.
 */
#ifndef SPDR_PROFILING_DISABLED 
	#define SPDR_METADATA1(spdr, name, arg0)				\
		UU_SPDR_TRACE1(spdr, "__metadata", name, SPDR_METADATA, arg0)
#else
	#define SPDR_METADATA1(spdr, name, arg0)
#endif

#endif
