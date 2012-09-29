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

int uu_spdr_musttrace(const struct spdr* context);

void uu_spdr_record(struct spdr* context,
		    const char* cat,
		    const char* name,
		    enum uu_spdr_type type);

void uu_spdr_record_1(struct spdr* context,
		      const char* cat,
		      const char* name,
		      enum uu_spdr_type type,
		      const char* key0,
		      const char* value0);

void uu_spdr_record_2(struct spdr* context,
		      const char* cat,
		      const char* name,
		      enum uu_spdr_type type,
		      const char* key0,
		      const char* value0,
		      const char* key1,
		      const char* value1);

#define UU_SPDR_COND_EXPR(cond, expr) \
	(void) (cond && ((expr), 1))

#define UU_SPDR_TRACE(context, cat, name, type)				\
	UU_SPDR_COND_EXPR(uu_spdr_musttrace(context),			\
			  uu_spdr_record(context, cat, name, type))

#define UU_SPDR_TRACE1(cat, name, type, key0, value0)			\
	UU_SPDR_COND_EXPR(uu_spdr_musttrace(context),			\
			  uu_spdr_record(context, cat, name, type))

#define UU_SPDR_TRACE2(cat, name, type, key0, value0, key1, value1)	\
	UU_SPDR_COND_EXPR(uu_spdr_musttrace(context),			\
			  uu_spdr_record(context, cat, name, type))

#endif
