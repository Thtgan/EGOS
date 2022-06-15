#if !defined(__V_PAGE_ALLOC)
#define __V_PAGE_ALLOC

#include<stddef.h>

void* vPageAlloc(size_t n);

void vPageFree(void* vPageBegin, size_t n);

#endif // __V_PAGE_ALLOC
