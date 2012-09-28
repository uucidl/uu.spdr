#ifndef UU_SPDR_H
#define UU_SPDR_H

#include "spdr-private.h"

/**
 * Context for the library
 */
struct spdr;

/**
 * Initializes the library
 */
int spdr_init(struct spdr **context);

/**
 * Provide your logging function if you want a trace stream to be produced.
 */
void spdr_set_log_fn(struct spdr *context,
		     void (*log_fn) (const char* line));

/**
 * Activates the recording of traces (off by default)
 */
void spdr_enable_trace(struct spdr *context, int traceon);

/**
 * An instant event
 */
#define SPDR_EVENT(spdr, cat, name)		\
	UU_SPDR_TRACE(spdr, cat, name, SPDR_EVENT)

/**
 * An instant event with one parameter
 */
#define SPDR_EVENT1(spdr, cat, name, key0, value0)		\
	UU_SPRD_TRACE1(spdr, cat, name, SPDR_EVENT, key0, value0)

/**
 * An instant event with two parameters
 */
#define SPDR_EVENT2(spdr, cat, name, key0, value0, key1, value1)	\
	UU_SPRD_TRACE2(spdr, cat, name, SPDR_EVENT, key0, value0, key1, value1)

/**
 * Begin a slice of work
 */
#define SPDR_BEGIN(spdr, cat, name)			\
	UU_SPDR_TRACE(spdr, cat, name, SPDR_BEGIN)

/**
 * Begin a slice of work, with one parameter.
 */
#define SPDR_BEGIN1(spdr, cat, name, key0, value0)			\
	UU_SPRD_TRACE1(spdr, cat, name, SPDR_BEGIN, key0, value0)

/**
 * Begin a slice of work, with two parameters
 */
#define SPDR_BEGIN2(spdr, cat, name, key0, value0, key1, value1)	\
	UU_SPRD_TRACE2(spdr, cat, name, SPDR_BEGIN, key0, value0, \
		       key1, value1)

/**
 * End a slice of work
 */
#define SPDR_END(spdr, cat, name)		\
	UU_SPDR_TRACE(spdr, cat, name, SPDR_END)

#endif
