#include<memory/memoryMap.h>

#include<kit/bit.h>
#include<kit/util.h>
#include<memory/memory.h>

MemoryMapEntry* memoryMap_searchEntry(MemoryMap* mMap, Range* r, Flags8 flags, Flags8 entryType) {
    Uint64 rBegin = r->begin, rEnd = r->begin + r->length;

    Uint8 searchType = EXTRACT_VAL(flags, 8, 0, 2);
    MemoryMapEntry* ret = NULL;
    if (TEST_FLAGS(flags, MEMORY_MAP_SEARCH_FLAG_TOPDOWN)) {
        for (int i = mMap->entryNum - 1; i >= 0; --i) {
            MemoryMapEntry* entry = &mMap->memoryMapEntries[i];
            if (entry->type != entryType) {
                continue;
            }

            Uint64 entryBegin = entry->base, entryEnd = entry->base + entry->length;

            if (!RANGE_HAS_OVERLAP(entryBegin, entryEnd, rBegin, rEnd)) {
                continue;
            }

            switch (searchType) {
                case MEMORY_MAP_SEARCH_TYPE_CONTAIN: {
                    if (RANGE_WITHIN(entryBegin, entryEnd, rBegin, rEnd, <=, <=)) {
                        return entry;
                    }
                    break;
                }
                case MEMORY_MAP_SEARCH_TYPE_WITHIN: {
                    if (RANGE_WITHIN(rBegin, rEnd, entryBegin, entryEnd, <=, <=)) {
                        return entry;
                    }
                    break;
                }
                case MEMORY_MAP_SEARCH_TYPE_OVERLAP: {
                    return entry;
                    break;
                }
                default: {
                    break;
                }
            }
        }
    } else {
        for (int i = 0; i < mMap->entryNum; ++i) {
            MemoryMapEntry* entry = &mMap->memoryMapEntries[i];
            if (entry->type != entryType) {
                continue;
            }
            
            Uint64 entryBegin = entry->base, entryEnd = entry->base + entry->length;

            if (!RANGE_HAS_OVERLAP(entryBegin, entryEnd, rBegin, rEnd)) {
                continue;
            }

            switch (searchType) {
                case MEMORY_MAP_SEARCH_TYPE_CONTAIN: {
                    if (RANGE_WITHIN(entryBegin, entryEnd, rBegin, rEnd, <=, <=)) {
                        return entry;
                    }
                    break;
                }
                case MEMORY_MAP_SEARCH_TYPE_WITHIN: {
                    if (RANGE_WITHIN(rBegin, rEnd, entryBegin, entryEnd, <=, <=)) {
                        return entry;
                    }
                    break;
                }
                case MEMORY_MAP_SEARCH_TYPE_OVERLAP: {
                    return entry;
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }

    return NULL;
}

Result memoryMap_splitEntry(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, MemoryMapEntry** newEntry) {
    Index32 index = ARRAY_POINTER_TO_INDEX(mMap->memoryMapEntries, entry);
    if (index == MEMORY_MAP_ENTRY_NUM - 1 || entry->length <= splitlength) {
        if (newEntry != NULL) {
            *newEntry = NULL;
        }

        return RESULT_FAIL;
    }

    Uint32 originalBase = entry->base, originalLength = entry->length;

    MemoryMapEntry* nextEntry = entry + 1;
    memory_memmove(nextEntry + 1, nextEntry, (mMap->entryNum - index - 1) * sizeof(MemoryMapEntry));

    nextEntry->base = entry->base + splitlength;
    nextEntry->length = entry->length - splitlength;
    nextEntry->type = entry->type;
    nextEntry->extendedAttributes = 0;

    if (newEntry != NULL) {
        *newEntry = nextEntry;
    }

    entry->length = splitlength;
    ++mMap->entryNum;

    return RESULT_SUCCESS;
}

Result memoryMap_combineNextEntry(MemoryMap* mMap, MemoryMapEntry* entry) {
    Index32 index = ARRAY_POINTER_TO_INDEX(mMap->memoryMapEntries, entry);
    MemoryMapEntry* nextEntry = entry + 1;
    if (index == mMap->entryNum - 1 || entry->type != nextEntry->type || entry->base + entry->length != nextEntry->base) {
        return RESULT_FAIL;
    }

    entry->length += nextEntry->length;
    memory_memmove(nextEntry, nextEntry + 1, (mMap->entryNum - index - 1) * sizeof(MemoryMapEntry));
    --mMap->entryNum;

    return RESULT_SUCCESS;
}

void memoryMap_tidyup(MemoryMap* mMap) {
    for (int i = 0; i < mMap->entryNum; ++i) {
        while (memoryMap_combineNextEntry(mMap, mMap->memoryMapEntries + i) == RESULT_SUCCESS);
    }
}

Result memoryMap_splitEntryAndTidyup(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, Uint8 splittedType, MemoryMapEntry** newEntry) {
    MemoryMapEntry* splitted;
    if (memoryMap_splitEntry(mMap, entry, splitlength, &splitted) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    splitted->type = splittedType;
    if (newEntry != NULL) {
        *newEntry = splitted;
    }

    memoryMap_tidyup(mMap);

    return RESULT_SUCCESS;
}