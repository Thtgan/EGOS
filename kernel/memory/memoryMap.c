#include<memory/memoryMap.h>

#include<kit/bit.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<debug.h>
#include<lib/errorPosix.h>

MemoryMapEntry* memoryMap_searchEntry(MemoryMap* mMap, Range* r, Flags8 flags, Flags8 entryType) {
    Uint64 rBegin = r->begin, rEnd = r->begin + r->length;

    Uint8 searchType = EXTRACT_VAL(flags, 8, 0, 2);
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

MemoryMapEntry* memoryMap_splitEntry(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength) {
    Index32 index = ARRAY_POINTER_TO_INDEX(mMap->memoryMapEntries, entry);
    if (index == MEMORY_MAP_ENTRY_NUM - 1) {
        ERROR_THROW_NEW(ERROR_OUT_OF_RESOURCES, error_out);
    }

    if (entry->length <= splitlength) {
        ERROR_THROW_NEW(ERROR_INVALID_ARGUMENT, error_out);
    }

    Uint32 originalBase = entry->base, originalLength = entry->length;

    MemoryMapEntry* nextEntry = entry + 1;
    memory_memmove(nextEntry + 1, nextEntry, (mMap->entryNum - index - 1) * sizeof(MemoryMapEntry));

    nextEntry->base = entry->base + splitlength;
    nextEntry->length = entry->length - splitlength;
    nextEntry->type = entry->type;
    nextEntry->extendedAttributes = 0;

    entry->length = splitlength;
    ++mMap->entryNum;

    return nextEntry;
error_out:
    return NULL;
}

bool memoryMap_combineNextEntry(MemoryMap* mMap, MemoryMapEntry* entry) {
    Index32 index = ARRAY_POINTER_TO_INDEX(mMap->memoryMapEntries, entry);
    MemoryMapEntry* nextEntry = entry + 1;
    if (!(index < mMap->entryNum - 1 && entry->type == nextEntry->type && entry->base + entry->length == nextEntry->base)) {
        return false;
    }

    entry->length += nextEntry->length;
    memory_memmove(nextEntry, nextEntry + 1, (mMap->entryNum - index - 1) * sizeof(MemoryMapEntry));
    --mMap->entryNum;

    return true;
}

void memoryMap_tidyup(MemoryMap* mMap) {
    int i = 0;
    while (i < mMap->entryNum - 1) {
        bool combined = true;
        while (combined) {
            combined = memoryMap_combineNextEntry(mMap, mMap->memoryMapEntries + i);
        }
        ++i;
    }
}

MemoryMapEntry* memoryMap_splitEntryAndTidyup(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, Uint8 splittedType) {
    MemoryMapEntry* splitted = memoryMap_splitEntry(mMap, entry, splitlength);
    if (splitted == NULL) {
        ERROR_THROW_NEW(ERROR_INVALID_STATE, error_out);
    }

    splitted->type = splittedType;

    memoryMap_tidyup(mMap);

    return splitted;
error_out:
    return NULL;
}