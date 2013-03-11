#ifndef UU_SPDR_PRIVATE_H
#define UU_SPDR_PRIVATE_H

/**
 * Context for the library
 */
struct spdr_context;

enum uu_spdr_type
{
	SPDR_EVENT = 'I',
	SPDR_BEGIN = 'B',
	SPDR_END = 'E',
	SPDR_METADATA = 'M',
	SPDR_COUNTER = 'C',
};

struct uu_spdr_arg
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
#  define UU_SPDR_INT(argkey, argvalue)   \
	((struct uu_spdr_arg) { .key = argkey, .type = SPDR_INT, .value = { .i = argvalue } })
#  define UU_SPDR_FLOAT(argkey, argvalue) \
	((struct uu_spdr_arg) { .key = argkey, .type = SPDR_FLOAT, .value = { .d = argvalue } })
#  define UU_SPDR_STR(argkey, argvalue) \
	((struct uu_spdr_arg) { .key = argkey, .type = SPDR_STR, .value = { .str = argvalue } })
#else
struct uu_spdr_arg uu_spdr_arg_make_int(const char* key, int value);
struct uu_spdr_arg uu_spdr_arg_make_double(const char* key, double value);
struct uu_spdr_arg uu_spdr_arg_make_str(const char* key, const char* str);

#  define UU_SPDR_INT(key, value)   uu_spdr_arg_make_int(key, value)
#  define UU_SPDR_FLOAT(key, value) uu_spdr_arg_make_double(key, value)
#  define UU_SPDR_STR(key, value)   uu_spdr_arg_make_str(key, value)
#endif


int uu_spdr_musttrace(const struct spdr_context* context);

void uu_spdr_record(struct spdr_context* context,
		    const char* cat,
		    const char* name,
		    enum uu_spdr_type type);

void uu_spdr_record_1(struct spdr_context* context,
		      const char* cat,
		      const char* name,
		      enum uu_spdr_type type,
		      struct uu_spdr_arg arg0);

void uu_spdr_record_2(struct spdr_context* context,
		      const char* cat,
		      const char* name,
		      enum uu_spdr_type type,
		      struct uu_spdr_arg arg0,
		      struct uu_spdr_arg arg1);

void uu_spdr_record_3(struct spdr_context* context,
		      const char* cat,
		      const char* name,
		      enum uu_spdr_type type,
		      struct uu_spdr_arg arg0,
		      struct uu_spdr_arg arg1,
		      struct uu_spdr_arg arg2);

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

#define UU_SPDR_TRACE3(context, cat, name, type, arg0, arg1, arg2)	\
	UU_SPDR_COND_EXPR(uu_spdr_musttrace(context),			\
			  uu_spdr_record_3(context, cat, name, type, arg0, arg1, arg2))

#if defined(__cplusplus) || (defined(__GNUC__) && !defined(__STRICT_ANSI__))

#  define UU_SPDR_CONCAT_N(prefix, suffix) prefix ## suffix
#  define UU_SPDR_CONCAT(prefix, suffix) UU_SPDR_CONCAT_N(prefix,suffix)

#  if defined(__cplusplus)

struct uu_spdr_scope
{
	struct spdr_context* spdr;
	const char* cat;
	const char* name;

	uu_spdr_scope(struct spdr_context* spdr, const char* cat, const char* name) :
	  spdr(spdr), cat(cat), name(name)
	{}

	~uu_spdr_scope()
	{
		UU_SPDR_TRACE(spdr, cat, name, SPDR_END);
	}
};

#    define UU_SPDR_SCOPE_SETUP(spdr, cat, name)			\
	struct uu_spdr_scope UU_SPDR_CONCAT(scope,__LINE__) (spdr, cat, name)

#  elif defined(__GNUC__) && !defined(__STRICT_ANSI__)
struct uu_spdr_scope
{
	struct spdr_context* spdr;
	const char* cat;
	const char* name;
};

static inline void uu_spdr_scope_exit (struct uu_spdr_scope* scope)
{
	UU_SPDR_TRACE(scope->spdr, scope->cat, scope->name, SPDR_END);
}

#    define UU_SPDR_SCOPE_SETUP(spdr, cat, name)			\
	struct uu_spdr_scope UU_SPDR_CONCAT(scope,__LINE__)			\
	  __attribute__((cleanup(uu_spdr_scope_exit))) = { spdr, cat, name }; (void) UU_SPDR_CONCAT(scope,__LINE__)

#  endif

#define UU_SPDR_SCOPE_TRACE(spdr, cat, name)		\
	UU_SPDR_SCOPE_SETUP(spdr, cat, name);		\
	UU_SPDR_TRACE(spdr, cat, name, SPDR_BEGIN)

#define UU_SPDR_SCOPE_TRACE1(spdr, cat, name, arg0)		\
	UU_SPDR_SCOPE_SETUP(spdr, cat, name);			\
	UU_SPDR_TRACE1(spdr, cat, name, SPDR_BEGIN, arg0)

#define UU_SPDR_SCOPE_TRACE2(spdr, cat, name, arg0, arg1)	\
	UU_SPDR_SCOPE_SETUP(spdr, cat, name);			\
	UU_SPDR_TRACE2(spdr, cat, name, SPDR_BEGIN, arg0, arg1)

#define UU_SPDR_SCOPE_TRACE3(spdr, cat, name, arg0, arg1, arg2)		\
	UU_SPDR_SCOPE_SETUP(spdr, cat, name);				\
	UU_SPDR_TRACE3(spdr, cat, name, SPDR_BEGIN, arg0, arg1, arg2)

#endif

#endif
