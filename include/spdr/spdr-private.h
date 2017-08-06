#ifndef UU_SPDR_PRIVATE_H
#define UU_SPDR_PRIVATE_H

/**
 * Context for the library
 */
struct SPDR_Context;

enum SPDR_Event_Type {
        SPDR_EVENT = 'I',
        SPDR_BEGIN = 'B',
        SPDR_END = 'E',
        SPDR_METADATA = 'M',
        SPDR_COUNTER = 'C',
        SPDR_ASYNC_EVENT_BEGIN = 'S',
        SPDR_ASYNC_EVENT_STEP = 'T',
        SPDR_ASYNC_EVENT_END = 'F'
};

enum SPDR_Event_Arg_Type { SPDR_INT, SPDR_FLOAT, SPDR_STR };

struct SPDR_Event_Arg {
        const char *key;
        enum SPDR_Event_Arg_Type type;
        union {
                int i;
                double d;
                const char *str;
        } value;
};

extern struct SPDR_Event_Arg uu_spdr_arg_make_int(const char *key, int value);
extern struct SPDR_Event_Arg uu_spdr_arg_make_double(const char *key,
                                                     double value);
extern struct SPDR_Event_Arg uu_spdr_arg_make_str(const char *key,
                                                  const char *str);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
/* type checks possible on c99 */
#define UU_SPDR_INT(argkey, argvalue)                                          \
        ((struct SPDR_Event_Arg){                                              \
            .key = argkey, .type = SPDR_INT, .value = {.i = argvalue}})
#define UU_SPDR_FLOAT(argkey, argvalue)                                        \
        ((struct SPDR_Event_Arg){                                              \
            .key = argkey, .type = SPDR_FLOAT, .value = {.d = argvalue}})
#define UU_SPDR_STR(argkey, argvalue)                                          \
        ((struct SPDR_Event_Arg){                                              \
            .key = argkey, .type = SPDR_STR, .value = {.str = argvalue}})
#else
#define UU_SPDR_INT(key, value) uu_spdr_arg_make_int(key, value)
#define UU_SPDR_FLOAT(key, value) uu_spdr_arg_make_double(key, value)
#define UU_SPDR_STR(key, value) uu_spdr_arg_make_str(key, value)
#endif

int uu_spdr_musttrace(const struct SPDR_Context *context);

void uu_spdr_record(struct SPDR_Context *context,
                    const char *cat,
                    const char *name,
                    enum SPDR_Event_Type type);

void uu_spdr_record_1(struct SPDR_Context *context,
                      const char *cat,
                      const char *name,
                      enum SPDR_Event_Type type,
                      struct SPDR_Event_Arg arg0);

void uu_spdr_record_2(struct SPDR_Context *context,
                      const char *cat,
                      const char *name,
                      enum SPDR_Event_Type type,
                      struct SPDR_Event_Arg arg0,
                      struct SPDR_Event_Arg arg1);

void uu_spdr_record_3(struct SPDR_Context *context,
                      const char *cat,
                      const char *name,
                      enum SPDR_Event_Type type,
                      struct SPDR_Event_Arg arg0,
                      struct SPDR_Event_Arg arg1,
                      struct SPDR_Event_Arg arg2);

#define UU_SPDR_COND_EXPR(cond, expr) (void)(cond && ((expr), 1))

#define UU_SPDR_TRACE(context, cat, name, type)                                \
        UU_SPDR_COND_EXPR(uu_spdr_musttrace(context),                          \
                          uu_spdr_record(context, cat, name, type))

#define UU_SPDR_TRACE1(context, cat, name, type, arg0)                         \
        UU_SPDR_COND_EXPR(uu_spdr_musttrace(context),                          \
                          uu_spdr_record_1(context, cat, name, type, arg0))

#define UU_SPDR_TRACE2(context, cat, name, type, arg0, arg1)                   \
        UU_SPDR_COND_EXPR(                                                     \
            uu_spdr_musttrace(context),                                        \
            uu_spdr_record_2(context, cat, name, type, arg0, arg1))

#define UU_SPDR_TRACE3(context, cat, name, type, arg0, arg1, arg2)             \
        UU_SPDR_COND_EXPR(                                                     \
            uu_spdr_musttrace(context),                                        \
            uu_spdr_record_3(context, cat, name, type, arg0, arg1, arg2))

#if defined(__cplusplus) || (defined(__GNUC__) && !defined(__STRICT_ANSI__))

#define UU_SPDR_CONCAT_N(prefix, suffix) prefix##suffix
#define UU_SPDR_CONCAT(prefix, suffix) UU_SPDR_CONCAT_N(prefix, suffix)

#if defined(__cplusplus)

struct SPDR_Scope {
        struct SPDR_Context *spdr;
        const char *cat;
        const char *name;
        ~SPDR_Scope() { UU_SPDR_TRACE(spdr, cat, name, SPDR_END); }
};

#define UU_SPDR_SCOPE_SETUP(spdr, cat, name)                                   \
        struct SPDR_Scope UU_SPDR_CONCAT(scope, __LINE__) = {spdr, cat, name}

#elif defined(__GNUC__) && !defined(__STRICT_ANSI__)
struct SPDR_Scope {
        struct SPDR_Context *spdr;
        const char *cat;
        const char *name;
};

static inline void SPDR_Scope_exit(struct SPDR_Scope *scope)
{
        UU_SPDR_TRACE(scope->spdr, scope->cat, scope->name, SPDR_END);
}

#define UU_SPDR_SCOPE_SETUP(spdr, cat, name)                                   \
        struct SPDR_Scope UU_SPDR_CONCAT(scope, __LINE__)                      \
            __attribute__((cleanup(SPDR_Scope_exit))) = {spdr, cat, name};     \
        (void)UU_SPDR_CONCAT(scope, __LINE__)

#endif

#define UU_SPDR_SCOPE_TRACE(spdr, cat, name)                                   \
        UU_SPDR_SCOPE_SETUP(spdr, cat, name);                                  \
        UU_SPDR_TRACE(spdr, cat, name, SPDR_BEGIN)

#define UU_SPDR_SCOPE_TRACE1(spdr, cat, name, arg0)                            \
        UU_SPDR_SCOPE_SETUP(spdr, cat, name);                                  \
        UU_SPDR_TRACE1(spdr, cat, name, SPDR_BEGIN, arg0)

#define UU_SPDR_SCOPE_TRACE2(spdr, cat, name, arg0, arg1)                      \
        UU_SPDR_SCOPE_SETUP(spdr, cat, name);                                  \
        UU_SPDR_TRACE2(spdr, cat, name, SPDR_BEGIN, arg0, arg1)

#define UU_SPDR_SCOPE_TRACE3(spdr, cat, name, arg0, arg1, arg2)                \
        UU_SPDR_SCOPE_SETUP(spdr, cat, name);                                  \
        UU_SPDR_TRACE3(spdr, cat, name, SPDR_BEGIN, arg0, arg1, arg2)

#endif

#endif
