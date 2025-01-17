#include<memory/memoryMap.h>

#include<kit/bit.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<debug.h>

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

Result* memoryMap_splitEntry(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, MemoryMapEntry** newEntry) {
    Index32 index = ARRAY_POINTER_TO_INDEX(mMap->memoryMapEntries, entry);
    if (index == MEMORY_MAP_ENTRY_NUM - 1) {
        if (newEntry != NULL) {
            *newEntry = NULL;
        }
        ERROR_THROW(ERROR_ID_OUT_OF_MEMORY);
    }

    if (entry->length <= splitlength) {
        if (newEntry != NULL) {
            *newEntry = NULL;
        }
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS);
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

    ERROR_RETURN_OK();
}

Result* memoryMap_combineNextEntry(MemoryMap* mMap, MemoryMapEntry* entry) {
    Index32 index = ARRAY_POINTER_TO_INDEX(mMap->memoryMapEntries, entry);
    MemoryMapEntry* nextEntry = entry + 1;
    if (index < mMap->entryNum - 1 && entry->type == nextEntry->type || entry->base + entry->length == nextEntry->base) {
        ERROR_THROW(ERROR_ID_DATA_ERROR);
    }

    entry->length += nextEntry->length;
    memory_memmove(nextEntry, nextEntry + 1, (mMap->entryNum - index - 1) * sizeof(MemoryMapEntry));
    --mMap->entryNum;

    ERROR_RETURN_OK();
}

void memoryMap_tidyup(MemoryMap* mMap) {
    int i = 0;
    while (i < mMap->entryNum - 1) {
        while (true) {
            Result* res = memoryMap_combineNextEntry(mMap, mMap->memoryMapEntries + i);
            if (ERROR_CHECK_ERROR(res)) {
                break;
            }
        }
        ++i;
    }

    ERROR_RESET();
}

Result* memoryMap_splitEntryAndTidyup(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, Uint8 splittedType, MemoryMapEntry** newEntry) {
    MemoryMapEntry* splitted;
    ERROR_TRY_CATCH_DIRECT(
        memoryMap_splitEntry(mMap, entry, splitlength, &splitted),
        ERROR_CATCH_DEFAULT_CODES_PASS
    );

    splitted->type = splittedType;
    if (newEntry != NULL) {
        *newEntry = splitted;
    }

    memoryMap_tidyup(mMap);

    ERROR_RETURN_OK();
}