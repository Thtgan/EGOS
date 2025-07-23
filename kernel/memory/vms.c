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
    REF_COUNTER_REFER(frames->refCounter);
}

static void __virtualMemoryRegion_initStruct(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr, VirtualMemoryRegionInfo* info, VirtualMemoryRegionSharedFrames* sharedFrames);

static void __virtualMemoryRegion_setFields(VirtualMemoryRegion* vmr, VirtualMemoryRegionInfo* info, VirtualMemoryRegionSharedFrames* sharedFrames);

static void __virtualMemoryRegion_clearFields(VirtualMemoryRegion* vmr);

static int __virtualMemoryRegion_compareFunc(RBtreeNode* node1, RBtreeNode* node2);

static int __virtualMemoryRegion_searchFunc(RBtreeNode* node, Object key);

static inline bool __virtualMemoryRegionInfo_isSimilar(VirtualMemoryRegionInfo* info1, VirtualMemoryRegionSharedFrames* sharedFrames1, VirtualMemoryRegionInfo* info2, VirtualMemoryRegionSharedFrames* sharedFrames2) {
    if (!(info1->flags == info2->flags && info1->memoryOperationsID == info2->memoryOperationsID)) {
        return false;
    }

    if (TEST_FLAGS(info1->flags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_SHARED)) {
        DEBUG_ASSERT_SILENT(sharedFrames1 != NULL);
        return sharedFrames1 == sharedFrames2;
    }

    if (info1->memoryOperationsID != DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE && info1->memoryOperationsID != DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED) {  //TODO: Pass if same file with correct offset
        return false;
    }
    
    return true;
}

static inline bool __virtualMemoryRegionInfo_isValid(VirtualMemoryRegionInfo* info) {
    Flags16 type = VIRTUAL_MEMORY_REGION_INFO_FLAGS_EXTRACT_TYPE(info->flags);
    if (type == VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_ANON && !(info->memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_PRIVATE || info->memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_SHARED)) {
        return false;
    }

    if (type == VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_FILE && !(info->memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE || info->memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED)) {
        return false;
    }
    
    return true;
}

static VirtualMemoryRegion* __virtualMemorySpace_createRegion(VirtualMemorySpace* vms, VirtualMemoryRegionInfo* info, VirtualMemoryRegionSharedFrames* sharedFrames);

static VirtualMemoryRegion* __virtualMemorySpace_split(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr, void* ptr);

static void __virtualMemorySpace_merge(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr1, VirtualMemoryRegion* vmr2);

static VirtualMemoryRegion* __virtualMemorySpace_splitToDraw(VirtualMemorySpace* vms, VirtualMemoryRegionInfo* wnatToDrawInfo, Size* mergeLengthRet);

static VirtualMemoryRegion* __virtualMemorySpace_mergeRange(VirtualMemorySpace* vms, VirtualMemoryRegion* beginRegion, Size mergeLength);

static inline bool __virtualMemorySpace_addRegion(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr) {
    DEBUG_ASSERT_SILENT(RANGE_WITHIN_PACKED(&vms->range, &vmr->info.range, <=, <=));

    RBtreeNode* node = RBtree_insert(&vms->regionTree, &vmr->treeNode);
    if (node != NULL) {
        return false;
    }
    ++vms->regionNum;
    return true;
}

static inline void __virtualMemorySpace_removeRegion(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr) {
    DEBUG_ASSERT_SILENT(RANGE_WITHIN_PACKED(&vms->range, &vmr->info.range, <=, <=));

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

    VirtualMemoryRegionInfo info = (VirtualMemoryRegionInfo) {
        .range = (Range) {
            .begin = (Uintptr)base,
            .length = length
        },
        .flags = VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_HOLE,
        .memoryOperationsID = 0,
        .file = NULL,
        .offset = 0
    };
    __virtualMemorySpace_createRegion(vms, &info, NULL);
}

void virtualMemorySpace_clearStruct(VirtualMemorySpace* vms) {
    for (RBtreeNode* node = RBtree_getFirst(&vms->regionTree); node != NULL;) {
        RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, node);
        VirtualMemoryRegion* vmr = HOST_POINTER(node, VirtualMemoryRegion, treeNode);
        Range* range = &vmr->info.range;
        virtualMemorySpace_erase(vms, (void*)range->begin, range->length);
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
    __virtualMemorySpace_removeRegion(des, firstRegion);
    mm_free(firstRegion);

    for (RBtreeNode* node = RBtree_getFirst(&src->regionTree); node != NULL; node = RBtree_getSuccessor(&src->regionTree, node)) {
        VirtualMemoryRegion* currentRegion = HOST_POINTER(node, VirtualMemoryRegion, treeNode);
        VirtualMemoryRegion* newRegion = mm_allocate(sizeof(VirtualMemoryRegion));
        if (newRegion == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        __virtualMemoryRegion_initStruct(des, newRegion, &currentRegion->info, currentRegion->sharedFrames);
        
        if (TEST_FLAGS(currentRegion->info.flags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_SHARED)) {
            DEBUG_ASSERT_SILENT(currentRegion->sharedFrames != NULL);
            __virtualMemoryRegionSharedFrames_refer(currentRegion->sharedFrames);
        }

        __virtualMemorySpace_addRegion(des, newRegion);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

VirtualMemoryRegion* virtualMemorySpace_draw(VirtualMemorySpace* vms, VirtualMemoryRegionInfo* info) {
    if (!__virtualMemoryRegionInfo_isValid(info)) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    Range* range = &info->range;
    DEBUG_ASSERT_SILENT(range->length % PAGE_SIZE == 0 && range->length > 0);
    DEBUG_ASSERT_SILENT(range->begin % PAGE_SIZE == 0);

    Size mergeLength = 0;
    VirtualMemoryRegion* beginRegion = __virtualMemorySpace_splitToDraw(vms, info, &mergeLength);
    ERROR_GOTO_IF_ERROR(0);

    __virtualMemoryRegion_clearFields(beginRegion);
    __virtualMemoryRegion_setFields(beginRegion, info, NULL);
    ERROR_GOTO_IF_ERROR(0);

    VirtualMemoryRegion* ret = __virtualMemorySpace_mergeRange(vms, beginRegion, mergeLength);

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
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
        Range* range = &region->info.range;
        Uintptr l1 = range->begin, r1 = range->begin + range->length;
        Uintptr l2 = (Uintptr)prefer, r2 = (Uintptr)prefer + length;

        if (RANGE_WITHIN(l1, r1, l2, r2, <=, <=)) {
            return prefer;
        }
    }

    DEBUG_ASSERT_SILENT(beginNode != NULL);
    for (RBtreeNode* currentNode = beginNode; currentNode != NULL; currentNode = RBtree_getSuccessor(&vms->regionTree, currentNode)) {
        VirtualMemoryRegion* currentRegion = HOST_POINTER(currentNode, VirtualMemoryRegion, treeNode);
        VirtualMemoryRegionInfo* info = &currentRegion->info;
        if (VIRTUAL_MEMORY_REGION_INFO_FLAGS_EXTRACT_TYPE(info->flags) == VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_HOLE && info->range.length >= length) {
            return (void*)info->range.begin;
        }
    }

    return NULL;
}

void virtualMemorySpace_erase(VirtualMemorySpace* vms, void* begin, Size length) {
    DEBUG_ASSERT_SILENT(length % PAGE_SIZE == 0 && length > 0);
    DEBUG_ASSERT_SILENT((Uintptr)begin % PAGE_SIZE == 0);

    VirtualMemoryRegionInfo info = (VirtualMemoryRegionInfo) {
        .range = (Range) {
            .begin = (Uintptr)begin,
            .length = length
        },
        .flags = VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_HOLE,
        .memoryOperationsID = 0,
        .file = NULL,
        .offset = 0
    };
    
    Size mergeLength = 0;
    VirtualMemoryRegion* beginRegion = __virtualMemorySpace_splitToDraw(vms, &info, &mergeLength);
    ERROR_GOTO_IF_ERROR(0);

    __virtualMemoryRegion_clearFields(beginRegion);

    __virtualMemorySpace_mergeRange(vms, beginRegion, mergeLength);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

VirtualMemoryRegion* virtualMemorySpace_getPrevRegion(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr) {
    RBtreeNode* prevNode = RBtree_getPredecessor(&vms->regionTree, &vmr->treeNode);
    return prevNode == NULL ? NULL : HOST_POINTER(prevNode, VirtualMemoryRegion, treeNode);
}

VirtualMemoryRegion* virtualMemorySpace_getNextRegion(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr) {
    RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, &vmr->treeNode);
    return nextNode == NULL ? NULL : HOST_POINTER(nextNode, VirtualMemoryRegion, treeNode);
}

static void __virtualMemoryRegionSharedFrames_initStruct(VirtualMemoryRegionSharedFrames* frames, Uintptr vBase, Size frameN) {
    frames->vBase = vBase;
    
    REF_COUNTER_INIT(frames->refCounter, 1);

    Size dividedN = DIVIDE_ROUND_UP(frameN, 2);
    vector_initStructN(&frames->frames, dividedN);
    for (int i = 0; i < dividedN; ++i) {
        vector_push(&frames->frames, INVALID_INDEX64);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __virtualMemoryRegionSharedFrames_derefer(VirtualMemoryRegionSharedFrames* frames) {
    if (REF_COUNTER_DEREFER(frames->refCounter) == 0) {
        return;
    }

    vector_clearStruct(&frames->frames);
}

static void __virtualMemoryRegion_initStruct(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr, VirtualMemoryRegionInfo* info, VirtualMemoryRegionSharedFrames* sharedFrames) {
    RBtreeNode_initStruct(&vms->regionTree, &vmr->treeNode);
    __virtualMemoryRegion_setFields(vmr, info, sharedFrames);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __virtualMemoryRegion_setFields(VirtualMemoryRegion* vmr, VirtualMemoryRegionInfo* info, VirtualMemoryRegionSharedFrames* sharedFrames) {
    VirtualMemoryRegionInfo* desInfo = &vmr->info;
    
    memory_memcpy(&desInfo->range, &info->range, sizeof(Range));
    desInfo->flags = info->flags;
    desInfo->memoryOperationsID = info->memoryOperationsID;

    Flags16 type = VIRTUAL_MEMORY_REGION_INFO_FLAGS_EXTRACT_TYPE(info->flags);
    if (type == VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_FILE) {
        DEBUG_ASSERT_SILENT(info->memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE || info->memoryOperationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED);
        desInfo->file = info->file;
        desInfo->offset = info->offset;
    } else {
        desInfo->file = NULL;
        desInfo->offset = 0;
    }

    if (TEST_FLAGS(info->flags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_SHARED)) {
        if (sharedFrames == NULL) {
            sharedFrames = mm_allocate(sizeof(VirtualMemoryRegionSharedFrames));
            if (sharedFrames == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }
            __virtualMemoryRegionSharedFrames_initStruct(sharedFrames, info->range.begin, info->range.length / PAGE_SIZE);
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
    VirtualMemoryRegionInfo* info = &vmr->info;
    if (TEST_FLAGS(info->flags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_SHARED)) {
        DEBUG_ASSERT_SILENT(vmr->sharedFrames != NULL);
        __virtualMemoryRegionSharedFrames_derefer(vmr->sharedFrames);
    }

    //TODO: Derefer file here?
    info->flags = VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_HOLE;
    info->memoryOperationsID = 0;
    info->file = NULL;
    info->offset = 0;
}

static int __virtualMemoryRegion_compareFunc(RBtreeNode* node1, RBtreeNode* node2) {
    VirtualMemoryRegion* vmr1 = HOST_POINTER(node1, VirtualMemoryRegion, treeNode), * vmr2 = HOST_POINTER(node2, VirtualMemoryRegion, treeNode);
    Range* range1 = &vmr1->info.range, * range2 = &vmr2->info.range;
    if (RANGE_HAS_OVERLAP(range1->begin, range1->begin + range1->length, range2->begin, range2->begin + range2->length)) {
        return 0;
    }

    return range1->begin < range2->begin ? -1 : 1;
}

static int __virtualMemoryRegion_searchFunc(RBtreeNode* node, Object key) {
    VirtualMemoryRegion* vmr = HOST_POINTER(node, VirtualMemoryRegion, treeNode);
    Range* range = &vmr->info.range;
    if (VALUE_WITHIN(range->begin, range->begin + range->length, (Uintptr)key, <=, <)) {
        return 0;
    }

    return range->begin < (Uintptr)key ? -1 : 1;
}

static VirtualMemoryRegion* __virtualMemorySpace_createRegion(VirtualMemorySpace* vms, VirtualMemoryRegionInfo* info, VirtualMemoryRegionSharedFrames* sharedFrames) {
    VirtualMemoryRegion* newRegion = mm_allocate(sizeof(VirtualMemoryRegion));
    if (newRegion == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    __virtualMemoryRegion_initStruct(vms, newRegion, info, sharedFrames);
    ERROR_GOTO_IF_ERROR(0);

    if (!__virtualMemorySpace_addRegion(vms, newRegion)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 1);
    }

    return newRegion;
    ERROR_FINAL_BEGIN(1);
    mm_free(newRegion);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static VirtualMemoryRegion* __virtualMemorySpace_split(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr, void* ptr) {
    VirtualMemoryRegionInfo* info = &vmr->info;
    Range* range = &info->range;
    DEBUG_ASSERT_SILENT(VALUE_WITHIN_PACKED(range, (Uintptr)ptr, <=, <=));
    if ((Uintptr)ptr == range->begin || (Uintptr)ptr == range->begin + range->length) {
        return NULL;
    }

    Size frontLength = (Uintptr)ptr - range->begin,
        backLength = range->length - frontLength;
    
    range->length = frontLength;

    VirtualMemoryRegionInfo newRegionInfo;
    memory_memcpy(&newRegionInfo, info, sizeof(VirtualMemoryRegionInfo));
    newRegionInfo.range = (Range) {
        .begin = (Uintptr)ptr,
        .length = backLength
    };

    Flags16 type = VIRTUAL_MEMORY_REGION_INFO_FLAGS_EXTRACT_TYPE(vmr->info.flags);
    if (type == VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_FILE) {
        newRegionInfo.offset = info->offset + frontLength;
    }

    VirtualMemoryRegion* ret = __virtualMemorySpace_createRegion(vms, &newRegionInfo, vmr->sharedFrames);
    ERROR_GOTO_IF_ERROR(0);

    return ret;
    ERROR_FINAL_BEGIN(0);
    range->length = frontLength + backLength;
    return NULL;
}

static void __virtualMemorySpace_merge(VirtualMemorySpace* vms, VirtualMemoryRegion* vmr1, VirtualMemoryRegion* vmr2) {
    Range* range1 = &vmr1->info.range, * range2 = &vmr2->info.range;
    DEBUG_ASSERT_SILENT(range1->begin + range1->length == range2->begin);
    Range* range = &vms->range;
    DEBUG_ASSERT_SILENT(RANGE_WITHIN(range->begin, range->begin + range->length, range1->begin, range2->begin + range2->length, <=, <=));
    
    __virtualMemoryRegion_clearFields(vmr2);
    
    __virtualMemorySpace_removeRegion(vms, vmr2);
    range1->length += range2->length;
    mm_free(vmr2);
}

static VirtualMemoryRegion* __virtualMemorySpace_splitToDraw(VirtualMemorySpace* vms, VirtualMemoryRegionInfo* wnatToDrawInfo, Size* mergeLengthRet) {
    Range* wantToDrawRange = &wnatToDrawInfo->range;
    Size mergeLength = wantToDrawRange->length;

    VirtualMemoryRegion* beginRegion = virtualMemorySpace_getRegion(vms, (void*)wantToDrawRange->begin);
    DEBUG_ASSERT_SILENT(beginRegion != NULL);
    if (!__virtualMemoryRegionInfo_isSimilar(&beginRegion->info, beginRegion->sharedFrames, wnatToDrawInfo, beginRegion->sharedFrames)) {
        VirtualMemoryRegion* newRegion = __virtualMemorySpace_split(vms, beginRegion, (void*)wantToDrawRange->begin);
        if (newRegion != NULL) {
            beginRegion = newRegion;
        }
    } else {
        Range* beginRange = &beginRegion->info.range;
        mergeLength = mergeLength - (beginRange->begin + beginRange->length - wantToDrawRange->begin) + beginRange->length;
    }

    VirtualMemoryRegion* endRegion = virtualMemorySpace_getRegion(vms, (void*)(wantToDrawRange->begin + wantToDrawRange->length - 1));
    DEBUG_ASSERT_SILENT(endRegion != NULL);
    if (!__virtualMemoryRegionInfo_isSimilar(&endRegion->info, endRegion->sharedFrames, wnatToDrawInfo, endRegion->sharedFrames)) {
        __virtualMemorySpace_split(vms, endRegion, (void*)(wantToDrawRange->begin + wantToDrawRange->length));
    } else {
        Range* endRange = &endRegion->info.range;
        mergeLength = mergeLength - (wantToDrawRange->begin + wantToDrawRange->length - endRange->begin) + endRange->length;
    }

    *mergeLengthRet = mergeLength;

    return beginRegion;
}

static VirtualMemoryRegion* __virtualMemorySpace_mergeRange(VirtualMemorySpace* vms, VirtualMemoryRegion* beginRegion, Size mergeLength) {
    DEBUG_ASSERT_SILENT(beginRegion->info.range.length <= mergeLength);
    Size remaining = mergeLength - beginRegion->info.range.length;
    while (remaining > 0) {
        RBtreeNode* nextNode = RBtree_getSuccessor(&vms->regionTree, &beginRegion->treeNode);
        VirtualMemoryRegion* nextRegion = HOST_POINTER(nextNode, VirtualMemoryRegion, treeNode);
        Size nextLength = nextRegion->info.range.length;
        DEBUG_ASSERT_SILENT(nextLength <= remaining);
        __virtualMemorySpace_merge(vms, beginRegion, nextRegion);

        remaining -= nextLength;
    }

    DEBUG_ASSERT_SILENT(remaining == 0);

    VirtualMemoryRegion* ret = beginRegion;

    VirtualMemoryRegion* nextRegion = virtualMemorySpace_getNextRegion(vms, beginRegion);
    if (nextRegion != NULL && __virtualMemoryRegionInfo_isSimilar(&beginRegion->info, beginRegion->sharedFrames, &nextRegion->info, nextRegion->sharedFrames)) {
        __virtualMemorySpace_merge(vms, beginRegion, nextRegion);
    }

    VirtualMemoryRegion* prevRegion = virtualMemorySpace_getPrevRegion(vms, beginRegion);
    if (prevRegion != NULL && __virtualMemoryRegionInfo_isSimilar(&prevRegion->info, prevRegion->sharedFrames, &beginRegion->info, beginRegion->sharedFrames)) {
        __virtualMemorySpace_merge(vms, prevRegion, beginRegion);
        ret = prevRegion;
    }

    return ret;
}