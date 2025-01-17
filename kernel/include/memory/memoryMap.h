#if !defined(__MEMORY_MEMORYMAP_H)
#define __MEMORY_MEMORYMAP_H

#include<system/memoryMap.h>
#include<kit/types.h>
#include<kit/bit.h>
#include<result.h>

#define MEMORY_MAP_SEARCH_TYPE_CONTAIN  0
#define MEMORY_MAP_SEARCH_TYPE_WITHIN   1
#define MEMORY_MAP_SEARCH_TYPE_OVERLAP  2
#define MEMORY_MAP_SEARCH_FLAG_TOPDOWN  FLAG8(2)

MemoryMapEntry* memoryMap_searchEntry(MemoryMap* mMap, Range* r, Flags8 flags, Flags8 entryType);

Result* memoryMap_splitEntry(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, MemoryMapEntry** newEntry);

Result* memoryMap_combineNextEntry(MemoryMap* mMap, MemoryMapEntry* entry);

void memoryMap_tidyup(MemoryMap* mMap);

Result* memoryMap_splitEntryAndTidyup(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, Uint8 splittedType, MemoryMapEntry** newEntry);

#endif // __MEMORY_MEMORYMAP_H
