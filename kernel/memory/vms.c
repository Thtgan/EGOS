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
    Flags64 ret = EMPTY_FLAGS;
    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_WRITABLE)) {
        SET_FLAG_BACK(ret, PAGING_ENTRY_FLAG_RW);
    }

    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_USER)) {
        SET_FLAG_BACK(ret, PAGING_ENTRY_FLAG_US);
    }
    
    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_NOT_EXECUTABLE)) {
        SET_FLAG_BACK(ret, PAGING_ENTRY_FLAG_XD);
    }

    return ret;
}

static int __virtualMemoryRegion_compareFunc(RBtreeNode* node1, RBtreeNode* node2);

static int __virtualMemoryRegion_searchFunc(RBtreeNode* node, Object key);

static inline bool __virtualMemoryRegion_check(VirtualMemoryRegion* vmr, Flags16 flags, Uint8 memoryOperationsID) {
    return vmr->flags == flags && vmr->memoryOperationsID == memoryOperationsID;
}

static VirtualMemoryRegion* __virtualMemorySpace_addHoleRegion(VirtualMemorySpace* vms, void* begin, Size length);

static VirtualMemoryRegion* __virtualMemorySpace_addRegion(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID);

static VirtualMemoryRegion* __virtualMemorySpace_split(VirtualMemorySpace* vms, void* ptr);

static void __virtualMemorySpace_merge(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr1, VirtualMemoryRegion* vmr2);

static void __virtualMemorySpace_doDraw(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID);

static inline bool __virtualMemorySpace_doAddRegion(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr) {
    DEBUG_ASSERT_SILENT(RANGE_WITHIN_PACKED(&vms->range, &vmr->range, <=, <=));

    RBtreeNode* node = RBtree_insert(&vms->regionTree, &vmr->treeNode);
    if (node != NULL) {
        return false;
    }
    ++vms->regionNum;
    return true;
}

static inline void __virtualMemorySpace_doRemoveRegion(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr) {
    DEBUG_ASSERT_SILENT(RANGE_WITHIN_PACKED(&vms->range, &vmr->range, <=, <=));

    RBtree_directDelete(&vms->regionTree, &vmr->treeNode);
    --vms->regionNum;
}

void virtualMemoryRegion_initStruct(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);
    
    RBtreeNode_initStruct(&vms->regionTree, &vmr->treeNode);
    vmr->range = (Range) {
        .begin = (Uintptr)begin,
        .length = length
    };

    vmr->flags = flags;
    vmr->memoryOperationsID = memoryOperationsID;
}

void virtualMemorySpace_initStruct(VirtualMemorySpace* vms, ExtendedPageTableRoot* pageTable, void* base, Size length) {
    RBtree_initStruct(&vms->regionTree, __virtualMemoryRegion_compareFunc, __virtualMemoryRegion_searchFunc);
    vms->regionNum = 0;
    vms->pageTable = pageTable;
    vms->range = (Range) {
        .begin = (Uintptr)base,
        .length = length
    };

    __virtualMemorySpace_addHoleRegion(vms, base, length);
}

void virtualMemorySpace_clearStruct(VirtualMemorySpace* vms) {
    for (RBtreeNode* node = RBtree_getFirst(&vms->regionTree); node != NULL;) {
        RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, node);
        VirtualMemoryRegion* vmr = HOST_POINTER(node, VirtualMemoryRegion, treeNode);
        virtualMemorySpace_erase(vms, (void*)vmr->range.begin, vmr->range.length);
        node = nextNode;
    }
}

void virtualMemorySpace_draw(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID) {
    __virtualMemorySpace_doDraw(vms, begin, length, flags, memoryOperationsID);
    ERROR_GOTO_IF_ERROR(0);

    Flags64 pagingProt = __virtualMemoryRegion_getPagingProt(flags);
    extendedPageTableRoot_draw(vms->pageTable, begin, NULL, length / PAGE_SIZE, memoryOperationsID, pagingProt, EXTENDED_PAGE_TABLE_DRAW_FLAGS_LAZY_MAP | EXTENDED_PAGE_TABLE_DRAW_FLAGS_ASSERT_DRAW_BLANK);
    ERROR_GOTO_IF_ERROR(0);
    
    return;
    ERROR_FINAL_BEGIN(0);
}

VirtualMemoryRegion* virtualMemorySpace_getRegion(VirtualMemorySpace* vms, void* ptr) {
    RBtreeNode* node = RBtree_search(&vms->regionTree, (Object)ptr);
    return node == NULL ? NULL : HOST_POINTER(node, VirtualMemoryRegion, treeNode);
}

void* virtualMemorySpace_findFirstFitHole(VirtualMemorySpace* vms, void* prefer, Size length) {
    DEBUG_ASSERT_SILENT(!RBtree_isEmpty(&vms->regionTree));
    
    RBtreeNode* beginNode = (prefer == NULL) ? NULL : RBtree_lowerBound(&vms->regionTree, (Object)prefer);
    if (beginNode == NULL) {
        beginNode = RBtree_getFirst(&vms->regionTree);
    } else {
        VirtualMemoryRegion* region = HOST_POINTER(beginNode, VirtualMemoryRegion, treeNode);
        Range* range = &region->range;
        Uintptr l1 = range->begin, r1 = range->begin + range->length;
        Uintptr l2 = (Uintptr)prefer, r2 = (Uintptr)prefer + length;

        if (RANGE_WITHIN(l1, r1, l2, r2, <=, <=)) {
            return prefer;
        }
    }

    DEBUG_ASSERT_SILENT(beginNode != NULL);
    for (RBtreeNode* currentNode = beginNode; currentNode != NULL; currentNode = RBtree_getSuccessor(&vms->regionTree, currentNode)) {
        VirtualMemoryRegion* currentRegion = HOST_POINTER(currentNode, VirtualMemoryRegion, treeNode);
        if (TEST_FLAGS(currentRegion->flags, VIRTUAL_MEMORY_REGION_FLAGS_HOLE) && currentRegion->range.length >= length) {
            return (void*)currentRegion->range.begin;
        }
    }

    return NULL;
}

void virtualMemorySpace_erase(VirtualMemorySpace* vms, void* begin, Size length) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);
    
    __virtualMemorySpace_doDraw(vms, begin, length, VIRTUAL_MEMORY_REGION_FLAGS_HOLE, 0);
    ERROR_GOTO_IF_ERROR(0);

    extendedPageTableRoot_erase(vms->pageTable, begin, length / PAGE_SIZE);

    return;
    ERROR_FINAL_BEGIN(0);
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

static VirtualMemoryRegion* __virtualMemorySpace_addHoleRegion(VirtualMemorySpace* vms, void* begin, Size length) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);
    
    VirtualMemoryRegion* newRegion = mm_allocate(sizeof(VirtualMemoryRegion));
    if (newRegion == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    virtualMemoryRegion_initStruct(vms, newRegion, begin, length, VIRTUAL_MEMORY_REGION_FLAGS_HOLE, 0);

    if (!__virtualMemorySpace_doAddRegion(vms, newRegion)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 1);
    }
    
    return newRegion;
    ERROR_FINAL_BEGIN(1);
    mm_free(newRegion);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static VirtualMemoryRegion* __virtualMemorySpace_addRegion(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);
    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(flags, VIRTUAL_MEMORY_REGION_FLAGS_HOLE));
    
    VirtualMemoryRegion* newRegion = mm_allocate(sizeof(VirtualMemoryRegion));
    if (newRegion == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    virtualMemoryRegion_initStruct(vms, newRegion, begin, length, flags, memoryOperationsID);

    if (!__virtualMemorySpace_doAddRegion(vms, newRegion)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 1);
    }

    return newRegion;
    ERROR_FINAL_BEGIN(1);
    mm_free(newRegion);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static VirtualMemoryRegion* __virtualMemorySpace_split(VirtualMemorySpace* vms, void* ptr) {
    VirtualMemoryRegion* vmr = virtualMemorySpace_getRegion(vms, ptr);
    if (vmr == NULL) {
        return NULL;
    }

    Range* range = &vmr->range;
    if ((Uintptr)ptr == range->begin || (Uintptr)ptr == range->begin + range->length) {
        return NULL;
    }

    Size frontLength = (Uintptr)ptr - range->begin,
        backLength = range->length - frontLength;
    
    range->length = frontLength;
    VirtualMemoryRegion* ret = NULL;
    if (TEST_FLAGS(vmr->flags, VIRTUAL_MEMORY_REGION_FLAGS_HOLE)) {
        ret = __virtualMemorySpace_addHoleRegion(vms, ptr, backLength);
    } else {
        ret = __virtualMemorySpace_addRegion(vms, ptr, backLength, vmr->flags, vmr->memoryOperationsID);
    }
    ERROR_GOTO_IF_ERROR(0);

    return ret;
    ERROR_FINAL_BEGIN(0);
    range->length = frontLength + backLength;
    return NULL;
}

static void __virtualMemorySpace_merge(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr1, VirtualMemoryRegion* vmr2) {
    Range* range1 = &vmr1->range, * range2 = &vmr2->range;
    DEBUG_ASSERT_SILENT(range1->begin + range1->length == range2->begin);
    Range* range = &vms->range;
    DEBUG_ASSERT_SILENT(RANGE_WITHIN(range->begin, range->begin + range->length, range1->begin, range2->begin + range2->length, <=, <=));

    __virtualMemorySpace_doRemoveRegion(vms, vmr2);
    range1->length += range2->length;
    mm_free(vmr2);
}

static void __virtualMemorySpace_doDraw(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);
    Range tmp = (Range) {
        .begin = (Uintptr)begin,
        .length = length
    };
    DEBUG_ASSERT_SILENT(RANGE_WITHIN_PACKED(&vms->range, &tmp, <=, <=));

    VirtualMemoryRegion* beginRegion = virtualMemorySpace_getRegion(vms, begin);
    DEBUG_ASSERT_SILENT(beginRegion != NULL);
    if (!__virtualMemoryRegion_check(beginRegion, flags, memoryOperationsID)) {
        VirtualMemoryRegion* newRegion = __virtualMemorySpace_split(vms, begin);
        if (newRegion != NULL) {
            beginRegion = newRegion;
        }
    }

    VirtualMemoryRegion* endRegion = virtualMemorySpace_getRegion(vms, begin + length - 1);
    DEBUG_ASSERT_SILENT(endRegion != NULL);
    if (!__virtualMemoryRegion_check(endRegion, flags, memoryOperationsID)) {
        __virtualMemorySpace_split(vms, begin + length);
    }

    beginRegion->flags = flags;
    beginRegion->memoryOperationsID = memoryOperationsID;
    if (beginRegion != endRegion) {
        DEBUG_ASSERT_SILENT(RANGE_WITHIN_PACKED(&tmp, &beginRegion->range, <=, <=));
        while (true) {
            RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, &beginRegion->treeNode);
            VirtualMemoryRegion* nextRegion = HOST_POINTER(nextNode, VirtualMemoryRegion, treeNode);
            DEBUG_ASSERT_SILENT(RANGE_WITHIN_PACKED(&tmp, &nextRegion->range, <=, <=));
            bool allDrawn = (nextRegion == endRegion);
            __virtualMemorySpace_merge(vms, beginRegion, nextRegion);
    
            if (allDrawn) {
                break;
            }
        }
    }

    RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, &beginRegion->treeNode);
    if (nextNode != NULL) {
        VirtualMemoryRegion* nextRegion = HOST_POINTER(nextNode, VirtualMemoryRegion, treeNode);
        if (__virtualMemoryRegion_check(nextRegion, flags, memoryOperationsID)) {
            __virtualMemorySpace_merge(vms, beginRegion, nextRegion);
        }
    }

    RBtreeNode* prevNode = RBtree_getPredecessor(&vms->regionTree, &beginRegion->treeNode);
    if (prevNode != NULL) {
        VirtualMemoryRegion* prevRegion = HOST_POINTER(prevNode, VirtualMemoryRegion, treeNode);
        if (__virtualMemoryRegion_check(prevRegion, flags, memoryOperationsID)) {
            __virtualMemorySpace_merge(vms, prevRegion, beginRegion);
        }
    }
}