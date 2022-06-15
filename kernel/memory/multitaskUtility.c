#include<memory/multitaskUtility.h>

#include<kit/bit.h>
#include<memory/memory.h>
#include<memory/paging/directAccess.h>
#include<memory/physicalMemory/pPageAlloc.h>
#include<system/pageTable.h>

PDPTable* __copyPDPTable(PDPTable* source);

PageDirectory* __copyPageDirectory(PageDirectory* source);

PageTable* __copyPageTable(PageTable* source);

PML4Table* copyPML4Table(PML4Table* source) {
    enableDirectAccess();
    PML4Table* ret = pPageAlloc(2);

    memcpy(ret->counters, source->counters, sizeof(source->counters));
    for (int i = 0; i < PML4_TABLE_SIZE; i++) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PML4_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_SHARE)) {
            ret->tableEntries[i] = source->tableEntries[i];
        } else {
            ret->tableEntries[i] = BUILD_PML4_ENTRY(__copyPDPTable(PDPT_ADDR_FROM_PML4_ENTRY(source->tableEntries[i])), FLAGS_FROM_PML4_ENTRY(source->tableEntries[i]));
        }
    }

    disableDirectAccess();

    return ret;
}

PDPTable* __copyPDPTable(PDPTable* source) {
    PDPTable* ret = pPageAlloc(1);

    for (int i = 0; i < PDP_TABLE_SIZE; i++) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PDPT_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_SHARE)) {
            ret->tableEntries[i] = source->tableEntries[i];
        } else {
            ret->tableEntries[i] = BUILD_PDPT_ENTRY(__copyPageDirectory(PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(source->tableEntries[i])), FLAGS_FROM_PDPT_ENTRY(source->tableEntries[i]));
        }
    }

    return ret;
}

PageDirectory* __copyPageDirectory(PageDirectory* source) {
    PageDirectory* ret = pPageAlloc(2);

    memcpy(ret->counters, source->counters, sizeof(source->counters));
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_SHARE)) {
            ret->tableEntries[i] = source->tableEntries[i];
        } else {
            ret->tableEntries[i] = BUILD_PAGE_DIRECTORY_ENTRY(__copyPageTable(PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(source->tableEntries[i])), FLAGS_FROM_PAGE_DIRECTORY_ENTRY(source->tableEntries[i]));
        }
    }

    return ret;
}

PageTable* __copyPageTable(PageTable* source) {
    PageTable* ret = pPageAlloc(1);

    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGE_TABLE_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_SHARE)) {
            ret->tableEntries[i] = source->tableEntries[i];
        } else {
            void* page = pPageAlloc(1);
            memcpy(page, PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(source->tableEntries[i]), PAGE_SIZE);
            ret->tableEntries[i] = BUILD_PAGE_TABLE_ENTRY(page, FLAGS_FROM_PAGE_DIRECTORY_ENTRY(source->tableEntries[i]));
        }
    }

    return ret;
}