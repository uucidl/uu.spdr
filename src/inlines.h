#if !defined(SPDR_INLINES_H)

#if __cplusplus
#define VOID_PTR_CAST(type, void_ptr) (type *)(void_ptr)
#else
#define VOID_PTR_CAST(type, void_ptr) void_ptr
#endif

#define SPDR_INLINES_H
#endif
