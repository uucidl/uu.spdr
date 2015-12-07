#if !defined(SPDR_INLINES_H)
#define SPDR_INLINES_H

#if __cplusplus
#define VOID_PTR_CAST(type, void_ptr) (type *)(void_ptr)
#else
#define VOID_PTR_CAST(type, void_ptr) void_ptr
#endif

#define spdr_internal static

/* Define non_aliasing type qualifier */
#if defined(_MSC_VER)
#if _MSC_VER > 1600
#define non_aliasing __restrict
#else
#define non_aliasing
#endif
#else
#define non_aliasing __restrict__
#endif

#endif
