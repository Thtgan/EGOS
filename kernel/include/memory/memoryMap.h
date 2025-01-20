#if !defined(__MEMORY_MEMORYMAP_H)
#define __MEMORY_MEMORYMAP_H

#include<system/memoryMap.h>
#include<kit/types.h>
#include<kit/bit.h>

#define MEMORY_MAP_SEARCH_TYPE_CONTAIN  0
#define MEMORY_MAP_SEARCH_TYPE_WITHIN   1
#define MEMORY_MAP_SEARCH_TYPE_OVERLAP  2
#define MEMORY_MAP_SEARCH_FLAG_TOPDOWN  FLAG8(2)

MemoryMapEntry* memoryMap_searchEntry(MemoryMap* mMap, Range* r, Flags8 flags, Flags8 entryType);

MemoryMapEntry* memoryMap_splitEntry(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength);

void memoryMap_combineNextEntry(MemoryMap* mMap, MemoryMapEntry* entry);

void memoryMap_tidyup(MemoryMap* mMap);

MemoryMapEntry* memoryMap_splitEntryAndTidyup(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, Uint8 splittedType);

#endif // __MEMORY_MEMORYMAP_H
