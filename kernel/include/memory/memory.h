#if !defined(__MEMORY_H)
#define __MEMORY_H

#include<stddef.h>
#include<stdint.h>
#include<system/memoryArea.h>

size_t initMemory(const struct MemoryMap* mMap);

void memset(void* ptr, uint8_t b, size_t n);

#endif // __MEMORY_H
