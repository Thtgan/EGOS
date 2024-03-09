#include<memory/paging/pagingCopy.h>

#include<kit/bit.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<system/pageTable.h>

/**
 * @brief Copy a new PDP table
 * 
 * @param source PDP table copy from
 * @return PDPtable* New PDP table
 */
static PDPtable* __copyPDPtable(PDPtable* source);

/**
 * @brief Copy a new page directory table
 * 
 * @param source Page directory copy from
 * @return PageDirectory* New PDP table
 */
static PageDirectory* __copyPageDirectory(PageDirectory* source);

/**
 * @brief Copy a new page table
 * 
 * @param source Page table copy from
 * @return PageTable* New page table
 */
static PageTable* __copyPageTable(PageTable* source);

PML4Table* copyPML4Table(PML4Table* source) {
    PML4Table* ret = physicalPage_alloc(1, physicalPage_getStruct(source)->attribute);
    source = convertAddressP2V(source);
    if (source == NULL || ret == NULL) {
        return NULL;
    }

    ret = convertAddressP2V(ret);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PAGING_ENTRY;
            continue;
        }

        void* mapTo = PAGING_TABLE_FROM_PAGING_ENTRY(source->tableEntries[i]);
        PhysicalPage* page = physicalPage_getStruct(mapTo);
        if (page == NULL) {
            return NULL;
        }

        switch (PHYSICAL_PAGE_TYPE_FROM_ATTRIBUTE(page->attribute)) {
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW: {
                CLEAR_FLAG_BACK(source->tableEntries[i], PAGING_ENTRY_FLAG_RW);
                ++page->cowCnt;
                if (TEST_FLAGS(source->tableEntries[i], PAGING_ENTRY_FLAG_PS)) {
                    ret->tableEntries[i] = source->tableEntries[i];
                }
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE: {
                ret->tableEntries[i] = source->tableEntries[i];
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE: {
                ret->tableEntries[i] = EMPTY_PAGING_ENTRY;
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED: {
                PDPtable* copied = __copyPDPtable((PDPtable*)mapTo);
                if (copied == NULL) {
                    return NULL;
                }
                ret->tableEntries[i] = BUILD_ENTRY_PAGING_TABLE(copied, FLAGS_FROM_PAGING_ENTRY(source->tableEntries[i]));
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_UNKNOWN:
            default:
                return NULL;
        }
    }

    return convertAddressV2P(ret);
}

static PDPtable* __copyPDPtable(PDPtable* source) {
    PDPtable* ret = physicalPage_alloc(1, physicalPage_getStruct(source)->attribute);
    source = convertAddressP2V(source);
    if (source == NULL || ret == NULL) {
        return NULL;
    }

    ret = convertAddressP2V(ret);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PAGING_ENTRY;
            continue;
        }

        void* mapTo = PAGING_TABLE_FROM_PAGING_ENTRY(source->tableEntries[i]);
        PhysicalPage* page = physicalPage_getStruct(mapTo);
        if (page == NULL) {
            return NULL;
        }

        switch (PHYSICAL_PAGE_TYPE_FROM_ATTRIBUTE(page->attribute)) {
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW: {
                CLEAR_FLAG_BACK(source->tableEntries[i], PAGING_ENTRY_FLAG_RW);
                ++page->cowCnt;
                if (TEST_FLAGS(source->tableEntries[i], PAGING_ENTRY_FLAG_PS)) {
                    ret->tableEntries[i] = source->tableEntries[i];
                }
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE: {
                ret->tableEntries[i] = source->tableEntries[i];
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE: {
                ret->tableEntries[i] = EMPTY_PAGING_ENTRY;
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED: {
                if (TEST_FLAGS(source->tableEntries[i], PAGING_ENTRY_FLAG_PS)) {
                    return NULL;
                } else {
                    PageDirectory* copied = __copyPageDirectory((PageDirectory*)mapTo);
                    if (copied == NULL) {
                        return NULL;
                    }
                    ret->tableEntries[i] = BUILD_ENTRY_PAGING_TABLE(copied, FLAGS_FROM_PAGING_ENTRY(source->tableEntries[i]));
                }
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_UNKNOWN:
            default:
                return NULL;
        }
    }

    return convertAddressV2P(ret);
}

static PageDirectory* __copyPageDirectory(PageDirectory* source) {
    PageDirectory* ret = physicalPage_alloc(1, physicalPage_getStruct(source)->attribute);
    source = convertAddressP2V(source);
    if (source == NULL || ret == NULL) {
        return NULL;
    }

    ret = convertAddressP2V(ret);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PAGING_ENTRY;
            continue;
        }

        void* mapTo = PAGING_TABLE_FROM_PAGING_ENTRY(source->tableEntries[i]);
        PhysicalPage* page = physicalPage_getStruct(mapTo);
        if (page == NULL) {
            return NULL;
        }

        switch(PHYSICAL_PAGE_TYPE_FROM_ATTRIBUTE(page->attribute)) {
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW: {
                CLEAR_FLAG_BACK(source->tableEntries[i], PAGING_ENTRY_FLAG_RW);
                ++page->cowCnt;
                if (TEST_FLAGS(source->tableEntries[i], PAGING_ENTRY_FLAG_PS)) {
                    ret->tableEntries[i] = source->tableEntries[i];
                }
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE: {
                ret->tableEntries[i] = source->tableEntries[i];
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE: {
                ret->tableEntries[i] = EMPTY_PAGING_ENTRY;
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED: {
                if (TEST_FLAGS(source->tableEntries[i], PAGING_ENTRY_FLAG_PS)) {
                    return NULL;
                } else {
                    PageTable* copied = __copyPageTable((PageTable*)mapTo);
                    if (copied == NULL) {
                        return NULL;
                    }
                    ret->tableEntries[i] = BUILD_ENTRY_PAGING_TABLE(copied, FLAGS_FROM_PAGING_ENTRY(source->tableEntries[i]));
                }
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_UNKNOWN:
            default:
                return NULL;
        }
    }

    return convertAddressV2P(ret);
}

static PageTable* __copyPageTable(PageTable* source) {
    PageTable* ret = physicalPage_alloc(1, physicalPage_getStruct(source)->attribute);
    source = convertAddressP2V(source);
    if (source == NULL || ret == NULL) {
        return NULL;
    }

    ret = convertAddressP2V(ret);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PAGING_ENTRY;
            continue;
        }

        void* mapTo = BASE_FROM_ENTRY_PS(PAGING_LEVEL_PAGE_TABLE, source->tableEntries[i]);
        PhysicalPage* page = physicalPage_getStruct(mapTo);
        if (page == NULL) {
            return NULL;
        }

        switch (PHYSICAL_PAGE_TYPE_FROM_ATTRIBUTE(page->attribute)) {
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW: {
                CLEAR_FLAG_BACK(source->tableEntries[i], PAGING_ENTRY_FLAG_RW);
                ++page->cowCnt;
                ret->tableEntries[i] = source->tableEntries[i];
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE: {
                ret->tableEntries[i] = source->tableEntries[i];
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE: {
                ret->tableEntries[i] = EMPTY_PAGING_ENTRY;
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED:
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_UNKNOWN:
            default:
                return NULL;
        }
    }

    return convertAddressV2P(ret);
}