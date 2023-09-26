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
    PML4Table* ret = pageAlloc(1, getPhysicalPageStruct(source)->attribute);                  
    source = convertAddressP2V(source);
    if (source == NULL || ret == NULL) {
        return NULL;
    }

    ret = convertAddressP2V(ret);
    for (int i = 0; i < PML4_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PML4_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PML4_ENTRY;
            continue;
        }

        void* mapTo = PDPT_ADDR_FROM_PML4_ENTRY(source->tableEntries[i]);
        PhysicalPage* page = getPhysicalPageStruct(mapTo);
        if (page == NULL) {
            return NULL;
        }

        switch (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute)) {
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW: {
                CLEAR_FLAG_BACK(source->tableEntries[i], PML4_ENTRY_FLAG_RW);
                ++page->cowCnt;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE: {
                ret->tableEntries[i] = source->tableEntries[i];
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE: {
                ret->tableEntries[i] = EMPTY_PML4_ENTRY;
            continue;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED: {
                PDPtable* copied = __copyPDPtable((PDPtable*)mapTo);
                if (copied == NULL) {
                    return NULL;
                }
                ret->tableEntries[i] = BUILD_PML4_ENTRY(copied, FLAGS_FROM_PML4_ENTRY(source->tableEntries[i]));
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
    PDPtable* ret = pageAlloc(1, getPhysicalPageStruct(source)->attribute);
    source = convertAddressP2V(source);
    if (source == NULL || ret == NULL) {
        return NULL;
    }

    ret = convertAddressP2V(ret);
    for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PDPT_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PDPT_ENTRY;
            continue;
        }

        void* mapTo = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(source->tableEntries[i]);
        PhysicalPage* page = getPhysicalPageStruct(mapTo);
        if (page == NULL) {
            return NULL;
        }

        switch (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute)) {
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW: {
                CLEAR_FLAG_BACK(source->tableEntries[i], PDPT_ENTRY_FLAG_RW);
                ++page->cowCnt;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE: {
                ret->tableEntries[i] = source->tableEntries[i];
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE: {
                ret->tableEntries[i] = EMPTY_PDPT_ENTRY;
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED: {
                if (TEST_FLAGS(source->tableEntries[i], PDPT_ENTRY_FLAG_PS)) {
                    return NULL;
                } else {
                    PageDirectory* copied = __copyPageDirectory((PageDirectory*)mapTo);
                    if (copied == NULL) {
                        return NULL;
                    }
                    ret->tableEntries[i] = BUILD_PDPT_ENTRY(copied, FLAGS_FROM_PDPT_ENTRY(source->tableEntries[i]));
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
    PageDirectory* ret = pageAlloc(1, getPhysicalPageStruct(source)->attribute);
    source = convertAddressP2V(source);
    if (source == NULL || ret == NULL) {
        return NULL;
    }

    ret = convertAddressP2V(ret);
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
            continue;
        }

        void* mapTo = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(source->tableEntries[i]);
        PhysicalPage* page = getPhysicalPageStruct(mapTo);
        if (page == NULL) {
            return NULL;
        }

        switch(PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute)) {
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW: {
                CLEAR_FLAG_BACK(source->tableEntries[i], PAGE_DIRECTORY_ENTRY_FLAG_RW);
                ++page->cowCnt;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE: {
                ret->tableEntries[i] = source->tableEntries[i];
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE: {
                ret->tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED: {
                if (TEST_FLAGS(source->tableEntries[i], PAGE_DIRECTORY_ENTRY_FLAG_PS)) {
                    return NULL;
                } else {
                    PageTable* copied = __copyPageTable((PageTable*)mapTo);
                    if (copied == NULL) {
                        return NULL;
                    }
                    ret->tableEntries[i] = BUILD_PAGE_DIRECTORY_ENTRY(copied, FLAGS_FROM_PAGE_DIRECTORY_ENTRY(source->tableEntries[i]));
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
    PageTable* ret = pageAlloc(1, getPhysicalPageStruct(source)->attribute);
    source = convertAddressP2V(source);
    if (source == NULL || ret == NULL) {
        return NULL;
    }

    ret = convertAddressP2V(ret);
    for (int i = 0; i < PAGE_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGE_TABLE_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PAGE_TABLE_ENTRY;
            continue;
        }

        void* mapTo = PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(source->tableEntries[i]);
        PhysicalPage* page = getPhysicalPageStruct(mapTo);
        if (page == NULL) {
            return NULL;
        }

        switch (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute)) {
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW: {
                CLEAR_FLAG_BACK(source->tableEntries[i], PAGE_TABLE_ENTRY_FLAG_RW);
                ++page->cowCnt;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE: {
                ret->tableEntries[i] = source->tableEntries[i];
                break;
            }
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE:
                ret->tableEntries[i] = EMPTY_PAGE_TABLE_ENTRY;
                break;
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED:
            case PHYSICAL_PAGE_ATTRIBUTE_TYPE_UNKNOWN:
            default:
                return NULL;
        }
    }

    return convertAddressV2P(ret);
}