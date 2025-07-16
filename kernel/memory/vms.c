#include<memory/vms.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memoryOperations.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<structs/RBtree.h>
#include<structs/refCounter.h>
#include<structs/vector.h>
#include<system/memoryLayout.h>
#include<system/pageTable.h>
#include<error.h>

static void __virtualMemoryRegionSharedFrames_initStruct(VirtualMemoryRegionSharedFrames* frames, Uintptr vBase, Size frameN);

static void __virtualMemoryRegionSharedFrames_derefer(VirtualMemoryRegionSharedFrames* frames);

static inline void __virtualMemoryRegionSharedFrames_refer(VirtualMemoryRegionSharedFrames* frames) {
    refCounter_refer(&frames->referCnt);
}

static void __virtualMemoryRegion_initStruct(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID, File* file, Index64 offset, VirtualMemoryRegionSharedFrames* sharedFrames);

static void __virtualMemoryRegion_setFields(VirtualMemoryRegion* vmr, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID, File* file, Index64 offset, VirtualMemoryRegionSharedFrames* sharedFrames);

static void __virtualMemoryRegion_clearFields(VirtualMemoryRegion* vmr);

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

static inline bool __virtualMemoryRegion_checkMatch(VirtualMemoryRegion* vmr, Flags16 flags, Uint8 memoryOperationsID, VirtualMemoryRegionSharedFrames* sharedFrames) {
    if (!(vmr->flags == flags && vmr->memoryOperationsID == memoryOperationsID)) {
        return false;
    }

    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_SHARED)) {
        return vmr->sharedFrames == sharedFrames;
    }

    return memoryOperationsID != DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE && memoryOperationsID != DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED;    //TODO: Pass if same file with correct offset
}

static VirtualMemoryRegion* __virtualMemorySpace_addHoleRegion(VirtualMemorySpace* vms, void* begin, Size length);

static VirtualMemoryRegion* __virtualMemorySpace_addFileRegion(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, File* file, Index64 offset, VirtualMemoryRegionSharedFrames* sharedFrames);

static VirtualMemoryRegion* __virtualMemorySpace_addAnonRegion(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID, VirtualMemoryRegionSharedFrames* sharedFrames);

static VirtualMemoryRegion* __virtualMemorySpace_split(VirtualMemorySpace* vms, void* ptr);

static void __virtualMemorySpace_merge(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr1, VirtualMemoryRegion* vmr2);

static VirtualMemoryRegion* __virtualMemorySpace_splitToDraw(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID);

static void __virtualMemorySpace_mergeRange(VirtualMemorySpace* vms, VirtualMemoryRegion* beginRegion, Size length, Flags16 flags, Uint8 memoryOperationsID);

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

Index32 virtualMemoryRegion_getFrameIndex(VirtualMemoryRegion* vmr, void* v) {
    VirtualMemoryRegionSharedFrames* sharedFrames = vmr->sharedFrames;
    if (sharedFrames == NULL) {
        return INVALID_INDEX32;
    }

    Index64 index = ((Uintptr)v - sharedFrames->vBase) / PAGE_SIZE;
    
    struct {
        union {
            Index32 indexes[2];
            Object obj;
        };
    } combined;
    combined.obj = vector_get(&sharedFrames->frames, index >> 1);
    return combined.indexes[index & 1];
}

Index32 virtualMemoryRegion_setFrameIndex(VirtualMemoryRegion* vmr, void* v, Index32 frameIndex) {
    VirtualMemoryRegionSharedFrames* sharedFrames = vmr->sharedFrames;
    if (sharedFrames == NULL) {
        return INVALID_INDEX32;
    }

    Index64 index = ((Uintptr)v - sharedFrames->vBase) / PAGE_SIZE;

    struct {
        union {
            Index32 indexes[2];
            Object obj;
        };
    } combined;
    combined.obj = vector_get(&sharedFrames->frames, index >> 1);
    combined.indexes[index & 1] = frameIndex;
    vector_set(&sharedFrames->frames, index >> 1, combined.obj);
}

void virtualMemorySpace_initStruct(VirtualMemorySpace* vms, ExtendedPageTableRoot* pageTable, void* base, Size length) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)base % PAGE_SIZE == 0);

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

void virtualMemorySpace_copy(VirtualMemorySpace* des, VirtualMemorySpace* src) {
    DEBUG_ASSERT_SILENT(des->regionNum == 1);
    DEBUG_ASSERT_SILENT(des->pageTable != NULL && des->pageTable != src->pageTable);

    memory_memcpy(&des->range, &src->range, sizeof(Range));
    
    RBtreeNode* firstNode = RBtree_getFirst(&des->regionTree);
    DEBUG_ASSERT_SILENT(firstNode != NULL);
    VirtualMemoryRegion* firstRegion = HOST_POINTER(firstNode, VirtualMemoryRegion, treeNode);
    __virtualMemorySpace_doRemoveRegion(des, firstRegion);
    mm_free(firstRegion);

    for (RBtreeNode* node = RBtree_getFirst(&src->regionTree); node != NULL; node = RBtree_getSuccessor(&src->regionTree, node)) {
        VirtualMemoryRegion* currentRegion = HOST_POINTER(node, VirtualMemoryRegion, treeNode);
        VirtualMemoryRegion* newRegion = mm_allocate(sizeof(VirtualMemoryRegion));
        if (newRegion == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        Range* range = &currentRegion->range;
        __virtualMemoryRegion_initStruct(des, newRegion, (void*)range->begin, range->length, currentRegion->flags, currentRegion->memoryOperationsID, currentRegion->file, currentRegion->offset, currentRegion->sharedFrames);
        
        if (TEST_FLAGS(currentRegion->flags, VIRTUAL_MEMORY_REGION_FLAGS_SHARED)) {
            DEBUG_ASSERT_SILENT(currentRegion->sharedFrames != NULL);
            __virtualMemoryRegionSharedFrames_refer(currentRegion->sharedFrames);
        }

        __virtualMemorySpace_doAddRegion(des, newRegion);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void virtualMemorySpace_drawAnon(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);

    VirtualMemoryRegion* beginRegion = __virtualMemorySpace_splitToDraw(vms, begin, length, flags, memoryOperationsID);

    __virtualMemoryRegion_clearFields(beginRegion);
    __virtualMemoryRegion_setFields(beginRegion, begin, length, flags, memoryOperationsID, NULL, 0, NULL);

    __virtualMemorySpace_mergeRange(vms, beginRegion, length, flags, memoryOperationsID);

    Flags64 pagingProt = __virtualMemoryRegion_getPagingProt(VIRTUAL_MEMORY_REGION_FLAGS_EXTRACT_PROT(flags));
    extendedPageTableRoot_draw(vms->pageTable, begin, NULL, length / PAGE_SIZE, memoryOperationsID, pagingProt, EXTENDED_PAGE_TABLE_DRAW_FLAGS_LAZY_MAP | EXTENDED_PAGE_TABLE_DRAW_FLAGS_ASSERT_DRAW_BLANK);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void virtualMemorySpace_drawFile(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, File* file, Index64 offset) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);

    Uint8 memoryOperationsID = TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_SHARED) ? DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED : DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE;

    VirtualMemoryRegion* beginRegion = __virtualMemorySpace_splitToDraw(vms, begin, length, flags, memoryOperationsID);

    __virtualMemoryRegion_clearFields(beginRegion);
    __virtualMemoryRegion_setFields(beginRegion, begin, length, flags, memoryOperationsID, file, offset, NULL);

    __virtualMemorySpace_mergeRange(vms, beginRegion, length, flags, memoryOperationsID);

    Flags64 pagingProt = __virtualMemoryRegion_getPagingProt(VIRTUAL_MEMORY_REGION_FLAGS_EXTRACT_PROT(flags));
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
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)prefer % PAGE_SIZE == 0);
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
    
    VirtualMemoryRegion* beginRegion = __virtualMemorySpace_splitToDraw(vms, begin, length, VIRTUAL_MEMORY_REGION_FLAGS_HOLE, 0);

    beginRegion->flags = VIRTUAL_MEMORY_REGION_FLAGS_HOLE;
    beginRegion->memoryOperationsID = 0;
    beginRegion->file = NULL;
    beginRegion->offset = 0;

    __virtualMemorySpace_mergeRange(vms, beginRegion, length, VIRTUAL_MEMORY_REGION_FLAGS_HOLE, 0);

    extendedPageTableRoot_erase(vms->pageTable, begin, length / PAGE_SIZE);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __virtualMemoryRegionSharedFrames_initStruct(VirtualMemoryRegionSharedFrames* frames, Uintptr vBase, Size frameN) {
    frames->vBase = vBase;
    
    refCounter_initStruct(&frames->referCnt);
    refCounter_refer(&frames->referCnt);

    Size dividedN = DIVIDE_ROUND_UP(frameN, 2);
    vector_initStructN(&frames->frames, dividedN);
    for (int i = 0; i < dividedN; ++i) {
        vector_push(&frames->frames, INVALID_INDEX64);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __virtualMemoryRegionSharedFrames_derefer(VirtualMemoryRegionSharedFrames* frames) {
    if (refCounter_derefer(&frames->referCnt)) {
        return;
    }

    vector_clearStruct(&frames->frames);
}

static void __virtualMemoryRegion_initStruct(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID, File* file, Index64 offset, VirtualMemoryRegionSharedFrames* sharedFrames) {
    RBtreeNode_initStruct(&vms->regionTree, &vmr->treeNode);
    __virtualMemoryRegion_setFields(vmr, begin, length, flags, memoryOperationsID, file, offset, sharedFrames);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __virtualMemoryRegion_setFields(VirtualMemoryRegion* vmr, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID, File* file, Index64 offset, VirtualMemoryRegionSharedFrames* sharedFrames) {
    vmr->range = (Range) {
        .begin = (Uintptr)begin,
        .length = length
    };
    
    vmr->flags = flags;
    vmr->memoryOperationsID = memoryOperationsID;

    if (memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE || memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED) {
        vmr->file = file;   //TODO: Refer file here?
        vmr->offset = offset;
    } else {
        vmr->file = NULL;
        vmr->offset = 0;
    }

    if (TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_SHARED)) {
        if (sharedFrames == NULL) {
            sharedFrames = mm_allocate(sizeof(VirtualMemoryRegionSharedFrames));
            if (sharedFrames == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }
            __virtualMemoryRegionSharedFrames_initStruct(sharedFrames, (Uintptr)begin, length / PAGE_SIZE);
            ERROR_GOTO_IF_ERROR(0);
        } else {
            __virtualMemoryRegionSharedFrames_refer(sharedFrames);
        }
        vmr->sharedFrames = sharedFrames;
    } else {
        vmr->sharedFrames = NULL;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __virtualMemoryRegion_clearFields(VirtualMemoryRegion* vmr) {
    if (TEST_FLAGS(vmr->flags, VIRTUAL_MEMORY_REGION_FLAGS_SHARED)) {
        DEBUG_ASSERT_SILENT(vmr->sharedFrames != NULL);
        __virtualMemoryRegionSharedFrames_derefer(vmr->sharedFrames);
    }

    //TODO: Derefer file here?
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
    VirtualMemoryRegion* newRegion = mm_allocate(sizeof(VirtualMemoryRegion));
    if (newRegion == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    __virtualMemoryRegion_initStruct(vms, newRegion, begin, length, VIRTUAL_MEMORY_REGION_FLAGS_HOLE, 0, NULL, 0, NULL);
    ERROR_GOTO_IF_ERROR(0);

    if (!__virtualMemorySpace_doAddRegion(vms, newRegion)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 1);
    }
    
    return newRegion;
    ERROR_FINAL_BEGIN(1);
    mm_free(newRegion);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static VirtualMemoryRegion* __virtualMemorySpace_addFileRegion(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, File* file, Index64 offset, VirtualMemoryRegionSharedFrames* sharedFrames) {
    VirtualMemoryRegion* newRegion = mm_allocate(sizeof(VirtualMemoryRegion));
    if (newRegion == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Uint8 memoryOperationsID = TEST_FLAGS(flags, VIRTUAL_MEMORY_REGION_FLAGS_SHARED) ? DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED : DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE;
    __virtualMemoryRegion_initStruct(vms, newRegion, begin, length, flags, memoryOperationsID, file, offset, sharedFrames);
    ERROR_GOTO_IF_ERROR(0);

    if (!__virtualMemorySpace_doAddRegion(vms, newRegion)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 1);
    }
    
    return newRegion;
    ERROR_FINAL_BEGIN(1);
    mm_free(newRegion);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static VirtualMemoryRegion* __virtualMemorySpace_addAnonRegion(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID, VirtualMemoryRegionSharedFrames* sharedFrames) {
    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(flags, VIRTUAL_MEMORY_REGION_FLAGS_HOLE) && memoryOperationsID != DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE && memoryOperationsID != DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED);
    
    VirtualMemoryRegion* newRegion = mm_allocate(sizeof(VirtualMemoryRegion));
    if (newRegion == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    __virtualMemoryRegion_initStruct(vms, newRegion, begin, length, flags, memoryOperationsID, NULL, 0, sharedFrames);
    ERROR_GOTO_IF_ERROR(0);

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
    } else if (vmr->memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE || vmr->memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED) {
        ret = __virtualMemorySpace_addFileRegion(vms, ptr, backLength, vmr->flags, vmr->file, vmr->offset + frontLength, vmr->sharedFrames);
    } else {
        ret = __virtualMemorySpace_addAnonRegion(vms, ptr, backLength, vmr->flags, vmr->memoryOperationsID, vmr->sharedFrames);
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
    
    __virtualMemoryRegion_clearFields(vmr2);
    
    __virtualMemorySpace_doRemoveRegion(vms, vmr2);
    range1->length += range2->length;
    mm_free(vmr2);
}

static VirtualMemoryRegion* __virtualMemorySpace_splitToDraw(VirtualMemorySpace* vms, void* begin, Size length, Flags16 flags, Uint8 memoryOperationsID) {
    VirtualMemoryRegion* beginRegion = virtualMemorySpace_getRegion(vms, begin);
    DEBUG_ASSERT_SILENT(beginRegion != NULL);
    if (!__virtualMemoryRegion_checkMatch(beginRegion, flags, memoryOperationsID, beginRegion->sharedFrames)) {
        VirtualMemoryRegion* newRegion = __virtualMemorySpace_split(vms, begin);
        if (newRegion != NULL) {
            beginRegion = newRegion;
        }
    }

    VirtualMemoryRegion* endRegion = virtualMemorySpace_getRegion(vms, begin + length - 1);
    DEBUG_ASSERT_SILENT(endRegion != NULL);
    if (!__virtualMemoryRegion_checkMatch(endRegion, flags, memoryOperationsID, endRegion->sharedFrames)) {
        __virtualMemorySpace_split(vms, begin + length);
    }

    return beginRegion;
}

static void __virtualMemorySpace_mergeRange(VirtualMemorySpace* vms, VirtualMemoryRegion* beginRegion, Size length, Flags16 flags, Uint8 memoryOperationsID) {
    DEBUG_ASSERT_SILENT(beginRegion->range.length <= length);
    Size remaining = length - beginRegion->range.length;
    while (remaining > 0) {
        RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, &beginRegion->treeNode);
        VirtualMemoryRegion* nextRegion = HOST_POINTER(nextNode, VirtualMemoryRegion, treeNode);
        Size nextLength = nextRegion->range.length;
        DEBUG_ASSERT_SILENT(nextLength <= remaining);
        __virtualMemorySpace_merge(vms, beginRegion, nextRegion);

        remaining -= nextLength;
    }

    DEBUG_ASSERT_SILENT(remaining == 0);

    RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, &beginRegion->treeNode);
    if (nextNode != NULL) {
        VirtualMemoryRegion* nextRegion = HOST_POINTER(nextNode, VirtualMemoryRegion, treeNode);
        if (__virtualMemoryRegion_checkMatch(nextRegion, flags, memoryOperationsID, nextRegion->sharedFrames)) {
            __virtualMemorySpace_merge(vms, beginRegion, nextRegion);
        }
    }

    RBtreeNode* prevNode = RBtree_getPredecessor(&vms->regionTree, &beginRegion->treeNode);
    if (prevNode != NULL) {
        VirtualMemoryRegion* prevRegion = HOST_POINTER(prevNode, VirtualMemoryRegion, treeNode);
        if (__virtualMemoryRegion_checkMatch(prevRegion, flags, memoryOperationsID, prevRegion->sharedFrames)) {
            __virtualMemorySpace_merge(vms, prevRegion, beginRegion);
        }
    }
}