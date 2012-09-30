#ifndef SPDR_PRIVATE_H
#define SPDR_PRIVATE_H

/**
 * Context for the library
 */
struct spdr;

enum uu_spdr_type
{
	SPDR_EVENT = 'I',
	SPDR_BEGIN = 'B',
	SPDR_END = 'E',
};

struct spdr_arg
{
	const char* key;
	enum { SPDR_INT, SPDR_FLOAT, SPDR_STR } type;
	union {
		int i;
		double d;
		const char* str;
	} value;
};

#if __STDC_VERSION__ >= 199901L
/* type checks possible on c99 */
#    define UU_SPDR_INT(argkey, argvalue)   \
	((struct spdr_arg) { .key = argkey, .type = SPDR_INT, .value = { .i = argvalue } })
#    define UU_SPDR_FLOAT(argkey, argvalue) \
	((struct spdr_arg) { .key = argkey, .type = SPDR_FLOAT, .value = { .d = argvalue } })
#    define UU_SPDR_STR(argkey, argvalue) \
	((struct spdr_arg) { .key = argkey, .type = SPDR_STR, .value = { .str = argvalue } })
#else
struct spdr_arg spdr_arg_make_int(const char* key, int value);
struct spdr_arg spdr_arg_make_double(const char* key, double value);
struct spdr_arg spdr_arg_make_str(const char* key, const char* str);

#    define UU_SPDR_INT(key, value)   spdr_arg_make_int(key, value)
#    define UU_SPDR_FLOAT(key, value) spdr_arg_make_double(key, value)
#    define UU_SPDR_STR(key, value)   spdr_arg_make_str(key, value)
#endif


int uu_spdr_musttrace(const struct spdr* context);

void uu_spdr_record(struct spdr* context,
		    const char* cat,
		    const char* name,
		    enum uu_spdr_type type);

void uu_spdr_record_1(struct spdr* context,
		      const char* cat,
		      const char* name,
		      enum uu_spdr_type type,
		      struct spdr_arg arg0);

void uu_spdr_record_2(struct spdr* context,
		      const char* cat,
		      const char* name,
		      enum uu_spdr_type type,
		      struct spdr_arg arg0,
		      struct spdr_arg arg1);

#define UU_SPDR_COND_EXPR(cond, expr) \
	(void) (cond && ((expr), 1))

#define UU_SPDR_TRACE(context, cat, name, type)				\
	UU_SPDR_COND_EXPR(uu_spdr_musttrace(context),			\
			  uu_spdr_record(context, cat, name, type))

#define UU_SPDR_TRACE1(context, cat, name, type, arg0)			\
	UU_SPDR_COND_EXPR(uu_spdr_musttrace(context),			\
			  uu_spdr_record_1(context, cat, name, type, arg0))

#define UU_SPDR_TRACE2(context, cat, name, type, arg0, arg1)		\
	UU_SPDR_COND_EXPR(uu_spdr_musttrace(context),			\
			  uu_spdr_record_2(context, cat, name, type, arg0, arg1))

#endif
