#if !defined(__E820_H)
#define __E820_H

#include<kit/types.h>
#include<system/memoryMap.h>

/**
 * @brief Initialize E820 entries
 * 
 * @param mMap Memory map to read to
 * 
 * @return num of detected memory area, -1 if error happened
 */
OldResult initE820(MemoryMap* mMap);

MemoryMapEntry* findE820Entry(MemoryMap* mMap, Uint32 base, Uint32 length, bool contain);

OldResult E820SplitEntry(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, MemoryMapEntry** newEntry);

OldResult E820CombineNextEntry(MemoryMap* mMap, MemoryMapEntry* entry);

void E820TidyUp(MemoryMap* mMap);

void printMemoryMap(MemoryMap* mMap);

#endif // __E820_H
