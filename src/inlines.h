#ifndef UU_SPDR_INLINES_H
#define UU_SPDR_INLINES_H

#if defined(__cplusplus)
#define SPDR_VOID_PTR_CAST(type, void_ptr) (type *)(void_ptr)
#else
#define SPDR_VOID_PTR_CAST(type, void_ptr) void_ptr
#endif

#if defined(__cplusplus) && defined(__has_cpp_attribute)
#define spdr_has_cpp_attribute(x) __has_cpp_attribute(x)
#else
#define spdr_has_cpp_attribute(x) 0
#endif

#if spdr_has_cpp_attribute(clang::fallthrough)
#define SPDR_FALLTHROUGH [[clang::fallthrough]]
#else
#define SPDR_FALLTHROUGH
#endif

#define spdr_internal static

/* Define non_aliasing type qualifier */
#if defined(_MSC_VER)
#if _MSC_VER > 1600
#define spdr_non_aliasing __restrict
#else
#define spdr_non_aliasing
#endif
#else
#define spdr_non_aliasing __restrict__
#endif

#endif
