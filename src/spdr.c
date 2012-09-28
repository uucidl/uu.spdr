#include "spdr.h"

/* needs a microseconds timestamp */
/* needs a process id */
/* needs a thread id */

extern int spdr_init(struct spdr **context)
{
	*context = 0;

	return -1;
}

/**
 * Provide your logging function if you want a trace stream to be produced.
 */
void spdr_set_log_fn(struct spdr *context,
		     void (*log_fn) (const char* line))
{

}

/**
 * Activates the recording of traces (off by default)
 */
void spdr_enable_trace(struct spdr *context, int traceon)
{

}

int uu_spdr_musttrace(struct spdr *context)
{
	return 0;
}

extern void uu_spdr_record(struct spdr *context,
			   const char* cat,
			   const char* name,
			   enum uu_spdr_type type)
{

}

extern void uu_spdr_record_1(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     const char* key0,
			     const char* value0)
{

}

extern void uu_spdr_record_2(struct spdr *context,
			     const char* cat,
			     const char* name,
			     enum uu_spdr_type type,
			     const char* key0,
			     const char* value0,
			     const char* key1,
			     const char* value1)
{

}
