#include<memory/vms.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<structs/RBtree.h>
#include<system/memoryLayout.h>
#include<system/pageTable.h>
#include<error.h>

static inline Flags64 __virtualMemoryRegion_getPagingProt(Flags16 flags) {
    Flags64 ret = PAGING_ENTRY_FLAG_US;
    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_WRITABLE)) {
        SET_FLAG_BACK(ret, PAGING_ENTRY_FLAG_RW);
    }
    
    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_NOT_EXECUTABLE)) {
        SET_FLAG_BACK(ret, PAGING_ENTRY_FLAG_XD);
    }

    return ret;
}

static int __virtualMemoryRegion_compareFunc(RBtreeNode* node1, RBtreeNode* node2);

static int __virtualMemoryRegion_searchFunc(RBtreeNode* node, Object key);

static VirtualMemoryRegion* __virtualMemorySpace_getFirstOverlapped(VirtualMemorySpace* vms, void* begin, Size length);

void virtualMemoryRegion_initStruct(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);
    
    RBtreeNode_initStruct(&vms->regionTree, &vmr->treeNode);
    vmr->range = (Range) {
        .begin = (Uintptr)begin,
        .length = length
    };

    vmr->flags = flags;
    vmr->memoryOperationsID = memoryOperationsID;
}

void virtualMemorySpace_initStruct(VirtualMemorySpace* vms, ExtendedPageTableRoot* pageTable) {
    RBtree_initStruct(&vms->regionTree, __virtualMemoryRegion_compareFunc, __virtualMemoryRegion_searchFunc);
    vms->regionNum = 0;
    vms->pageTable = pageTable;
}

void virtualMemorySpace_clearStruct(VirtualMemorySpace* vms) {
    for (RBtreeNode* node = RBtree_getFirst(&vms->regionTree); node != NULL;) {
        RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, node);
        virtualMemorySpace_removeRegion(vms, HOST_POINTER(node, VirtualMemoryRegion, treeNode));
        node = nextNode;
    }
}

VirtualMemoryRegion* virtualMemorySpace_addRegion(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);
    
    VirtualMemoryRegion* newRegion = mm_allocate(sizeof(VirtualMemoryRegion));
    if (newRegion == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    virtualMemoryRegion_initStruct(vms, newRegion, begin, length, flags, memoryOperationsID);
    RBtreeNode* node = RBtree_insert(&vms->regionTree, &newRegion->treeNode);
    if (node != NULL) {
        mm_free(newRegion);
        return HOST_POINTER(node, VirtualMemoryRegion, treeNode);
    }

    Flags64 pagingProt = __virtualMemoryRegion_getPagingProt(flags);
    extendedPageTableRoot_draw(vms->pageTable, begin, NULL, length / PAGE_SIZE, memoryOperationsID, pagingProt, EXTENDED_PAGE_TABLE_DRAW_FLAGS_LAZY_MAP | EXTENDED_PAGE_TABLE_DRAW_FLAGS_ASSERT_DRAW_BLANK);
    ERROR_GOTO_IF_ERROR(0);
    
    ++vms->regionNum;
    return NULL;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

VirtualMemoryRegion* virtualMemorySpace_getRegion(VirtualMemorySpace* vms, void* ptr) {
    RBtreeNode* node = RBtree_search(&vms->regionTree, (Object)ptr);
    return node == NULL ? NULL : HOST_POINTER(node, VirtualMemoryRegion, treeNode);
}

void* virtualMemorySpace_getFirstFit(VirtualMemorySpace* vms, void* prefer, Size length) {
    if (RBtree_isEmpty(&vms->regionTree))  {
        return (void*)MEMORY_LAYOUT_VMS_SPACE_BEGIN;
    }
    
    VirtualMemoryRegion* currentRegion = NULL;
    if (prefer == NULL) {
        currentRegion = HOST_POINTER(RBtree_getFirst(&vms->regionTree), VirtualMemoryRegion, treeNode);
        if (MEMORY_LAYOUT_VMS_SPACE_BEGIN + length <= currentRegion->range.begin) {
            return (void*)MEMORY_LAYOUT_VMS_SPACE_BEGIN;
        }
    } else {
        RBtreeNode* firstNode = RBtree_lowerBound(&vms->regionTree, (Object)prefer);
        if (firstNode == NULL) {
            VirtualMemoryRegion* region = HOST_POINTER(RBtree_getFirst(&vms->regionTree), VirtualMemoryRegion, treeNode);
            if (region->range.begin > (Uintptr)prefer + length) {
                return prefer;
            }
            currentRegion = region;
        } else {
            currentRegion = HOST_POINTER(firstNode, VirtualMemoryRegion, treeNode);
        }
    }

    void* currentAddr = (void*)(currentRegion->range.begin + currentRegion->range.length);
    while (true) {
        RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, &currentRegion->treeNode);
        if (nextNode == NULL) {
            break;
        }
        
        VirtualMemoryRegion* nextRegion = HOST_POINTER(nextNode, VirtualMemoryRegion, treeNode);
        Range* nextRange = &nextRegion->range;
        if (nextRange->begin >= (Uintptr)currentAddr + length) {
            return currentAddr;
        }
        currentAddr = (void*)(nextRange->begin + nextRange->length);

        currentRegion = nextRegion;
    }
    return currentAddr;
}

void virtualMemorySpace_removeRegion(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr) {
    RBtree_directDelete(&vms->regionTree, &vmr->treeNode);
    --vms->regionNum;
    extendedPageTableRoot_erase(vms->pageTable, (void*)vmr->range.begin, vmr->range.length / PAGE_SIZE);
    mm_free(vmr);
}

void virtualMemorySpace_split(VirtualMemorySpace* vms, void* ptr) {
    VirtualMemoryRegion* vmr = virtualMemorySpace_getRegion(vms, ptr);
    if (vmr == NULL) {
        return;
    }

    Range* range = &vmr->range;
    if ((Uintptr)ptr == range->begin || (Uintptr)ptr == range->begin + range->length) {
        return;
    }

    Size frontLength = (Uintptr)ptr - range->begin,
        backLength = range->length - frontLength;
    
    range->length = frontLength;
    DEBUG_ASSERT_SILENT(virtualMemorySpace_addRegion(vms, ptr, backLength, vmr->flags, vmr->memoryOperationsID) == NULL);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
    range->length = frontLength + backLength;
}

void virtualMemorySpace_erase(VirtualMemorySpace* vms, void* begin, Size length) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);

    VirtualMemoryRegion* currentRegion = __virtualMemorySpace_getFirstOverlapped(vms, begin, length);
    if (currentRegion == NULL) {
        return;
    }

    Uintptr l1 = (Uintptr)begin, r1 = (Uintptr)begin + length;
    while (currentRegion != NULL) {
        Range* currentRange = &currentRegion->range;
        Uintptr l2 = currentRange->begin, r2 = currentRange->begin + currentRange->length;
        if (!RANGE_HAS_OVERLAP(l1, r1, l2, r2)) {
            break;
        }

        RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, &currentRegion->treeNode);

        bool lCovered = l1 <= l2, rCovered = r2 <= r1;
        DEBUG_ASSERT_SILENT(lCovered || rCovered);
        
        if (lCovered && rCovered) {
            virtualMemorySpace_removeRegion(vms, currentRegion);
        } else if (lCovered) {  //TODO: Move single bound covered out of loop
            DEBUG_ASSERT_SILENT(VALUE_WITHIN(currentRange->begin, currentRange->begin + currentRange->length, r1, <, <));
            extendedPageTableRoot_erase(vms->pageTable, (void*)currentRange->begin, (r1 - currentRange->begin) / PAGE_SIZE);
            Size newLength = currentRange->begin + currentRange->length - r1;
            currentRange->begin = r1;
            currentRange->length = newLength;
        } else if (rCovered) {
            DEBUG_ASSERT_SILENT(VALUE_WITHIN(currentRange->begin, currentRange->begin + currentRange->length, l1, <, <));
            extendedPageTableRoot_erase(vms->pageTable, (void*)l1, (currentRange->begin + currentRange->length - l1) / PAGE_SIZE);
            currentRange->length = l1 - currentRange->begin;
        }
        
        currentRegion = HOST_POINTER(nextNode, VirtualMemoryRegion, treeNode);
    }
}

static int __virtualMemoryRegion_compareFunc(RBtreeNode* node1, RBtreeNode* node2) {
    VirtualMemoryRegion* vmr1 = HOST_POINTER(node1, VirtualMemoryRegion, treeNode), * vmr2 = HOST_POINTER(node2, VirtualMemoryRegion, treeNode);
    Range* range1 = &vmr1->range, * range2 = &vmr2->range;
    if (RANGE_HAS_OVERLAP(range1->begin, range1->begin + range1->length, range2->begin, range2->begin + range2->length)) {
        return 0;
    }

    return range1->begin < range2->begin ? -1 : 1;
}

static int __virtualMemoryRegion_searchFunc(RBtreeNode* node, Object key) {
    VirtualMemoryRegion* vmr = HOST_POINTER(node, VirtualMemoryRegion, treeNode);
    Range* range = &vmr->range;
    if (VALUE_WITHIN(range->begin, range->begin + range->length, (Uintptr)key, <=, <)) {
        return 0;
    }

    return range->begin < (Uintptr)key ? -1 : 1;
}

static VirtualMemoryRegion* __virtualMemorySpace_getFirstOverlapped(VirtualMemorySpace* vms, void* begin, Size length) {
    if (RBtree_isEmpty(&vms->regionTree)) {
        return NULL;
    }
    
    RBtreeNode* firstNode = RBtree_lowerBound(&vms->regionTree, (Object)begin);
    if (firstNode == NULL) {                                                        //No node is ahead or cover the begin
        firstNode = RBtree_getFirst(&vms->regionTree);
    } else if (__virtualMemoryRegion_searchFunc(firstNode, (Object)begin) != 0) {   //firstNode is first node ahead the begin
        firstNode = RBtree_getSuccessor(&vms->regionTree, firstNode);               //firstNode might be NULL heere
    }

    if (firstNode == NULL) {
        return NULL;
    }

    VirtualMemoryRegion* region = HOST_POINTER(firstNode, VirtualMemoryRegion, treeNode);
    //firstNode must be behind the begin
    Range* range = &region->range;
    return RANGE_HAS_OVERLAP(range->begin, range->begin + range->length, (Uintptr)begin, length) ? region : NULL;
}