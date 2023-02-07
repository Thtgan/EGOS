#if !defined(__PAGING_COPY_H)
#define __PAGING_COPY_H

#include<system/pageTable.h>

PML4Table* copyPML4Table(PML4Table* source);

#endif // __PAGING_COPY_H
