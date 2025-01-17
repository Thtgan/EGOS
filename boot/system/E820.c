#include<system/E820.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<lib/intn.h>
#include<lib/memory.h>
#include<lib/print.h>
#include<real/flags/eflags.h>
#include<system/memoryMap.h>
#include<system/systemInfo.h>

#define SMAP 0x534D4150

OldResult initE820(MemoryMap* mMap) {
    IntRegisters regs;
    MemoryMapEntry* table = mMap->memoryMapEntries;
    
    initRegs(&regs);
    regs.ebx = 0;

    int cnt = 0;
    do {
        regs.eax = 0xE820;
        regs.ecx = sizeof(MemoryMapEntry);
        regs.edx = SMAP;
        regs.edi = (Uint32)&table[cnt];
        intn(0x15, &regs, &regs);

        if (TEST_FLAGS(regs.eflags, EFLAGS_CF)) {
            return RESULT_ERROR;
        }
    } while(regs.ebx != 0 && ++cnt < MEMORY_MAP_ENTRY_NUM);

    mMap->entryNum = cnt;

    return RESULT_SUCCESS;
}

MemoryMapEntry* findE820Entry(MemoryMap* mMap, Uint32 base, Uint32 length, bool contain) {
    Uint32 n = mMap->entryNum, end = base + length;
    for (int i = 0; i < n; ++i) {
        MemoryMapEntry* entry = &mMap->memoryMapEntries[i];
        Uint32 entryEnd = entry->base + entry->length;

        if (!(entry->base <= base && base < entryEnd)) {
            continue;
        }

        if (contain) {
            return entry->base <= end && end < entryEnd ? entry : NULL;
        } else {
            return entry;
        }
    }

    return NULL;
}

OldResult E820SplitEntry(MemoryMap* mMap, MemoryMapEntry* entry, Size splitlength, MemoryMapEntry** newEntry) {
    Index32 index = ARRAY_POINTER_TO_INDEX(mMap->memoryMapEntries, entry);
    if (index == MEMORY_MAP_ENTRY_NUM - 1 || entry->length <= splitlength) {
        if (newEntry != NULL) {
            *newEntry = NULL;
        }

        return RESULT_ERROR;
    }

    Uint32 originalBase = entry->base, originalLength = entry->length;

    MemoryMapEntry* nextEntry = entry + 1;
    memmove(nextEntry + 1, nextEntry, (mMap->entryNum - index - 1) * sizeof(MemoryMapEntry));
    ++mMap->entryNum;

    nextEntry->base = entry->base + splitlength;
    nextEntry->length = entry->length - splitlength;
    nextEntry->type = entry->type;
    nextEntry->extendedAttributes = 0;

    if (newEntry != NULL) {
        *newEntry = nextEntry;
    }

    entry->length = splitlength;

    printf(
        "Split E820 entry: (%#010X, %#010X)\n -> (%#010X, %#010X), (%#010X, %#010X)\n",
        originalBase, originalLength, (Uint32)entry->base, (Uint32)entry->length, (Uint32)nextEntry->base, (Uint32)nextEntry->length
        );

    return RESULT_SUCCESS;
}

OldResult E820CombineNextEntry(MemoryMap* mMap, MemoryMapEntry* entry) {
    Index32 index = ARRAY_POINTER_TO_INDEX(mMap->memoryMapEntries, entry);
    MemoryMapEntry* nextEntry = entry + 1;
    if (index == mMap->entryNum - 1 || entry->type != nextEntry->type || entry->base + entry->length != nextEntry->base) {
        return RESULT_ERROR;
    }

    Uint32 base1 = (Uint32)entry->base, length1 = (Uint32)entry->length, base2 = (Uint32)nextEntry->base, length2 = (Uint32)nextEntry->length;
    memmove(nextEntry, nextEntry + 1, (mMap->entryNum - index - 1) * sizeof(MemoryMapEntry));
    --mMap->entryNum;

    entry->length = length1 + length2;

    printf(
        "Combine E820 entry: (%#010X, %#010X), (%#010X, %#010X)\n -> (%#010X, %#010X)\n",
        base1, length1, base2, length2, (Uint32)entry->base, (Uint32)entry->length
        );

    return RESULT_SUCCESS;
}

void E820TidyUp(MemoryMap* mMap) {
    for (int i = 0; i < mMap->entryNum; ++i) {
        while (E820CombineNextEntry(mMap, mMap->memoryMapEntries + i) == RESULT_SUCCESS);
    }
}

void printMemoryMap(MemoryMap* mMap) {
    printf("%d memory areas detected\n", mMap->entryNum);
    print("| Base Address | Area Length | Type |\n");
    for (int i = 0; i < mMap->entryNum; ++i) {
        const MemoryMapEntry* e = mMap->memoryMapEntries + i;
        printf("|  %#010X  | %#010X  | %#04X |\n", (Uint32)e->base, (Uint32)e->length, e->type);
    }
}