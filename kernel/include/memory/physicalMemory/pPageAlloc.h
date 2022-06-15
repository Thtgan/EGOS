#if !defined(__P_PAGE_ALLOC)
#define __P_PAGE_ALLOC

#include<stdbool.h>
#include<stddef.h>
#include<system/systemInfo.h>

void initPpageAlloc(const SystemInfo* sysInfo);

void* pPageAlloc(size_t n);

void pPageFree(void* pPageBegin, size_t n);

#endif // __P_PAGE_ALLOC
