#include<memory/frameMetadata.h>

#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocators/allocator.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<structs/linkedList.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>
#include<debug.h>

#define __FRAME_METADATA_PAGE_N(__N)    DIVIDE_ROUND_UP((sizeof(FrameMetadataHeader) + sizeof(FrameMetadata) * (__N)), PAGE_SIZE)

void frameMetadataHeader_initStruct(FrameMetadataHeader* header, void* frames, Size n) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(frames));
    
    Size metadataPageNum = __FRAME_METADATA_PAGE_N(n);
    DEBUG_ASSERT_SILENT(metadataPageNum < n);

    linkedListNode_initStruct(&header->node);
    header->frameBaseIndex = FRAME_METADATA_FRAME_TO_INDEX(frames) + metadataPageNum;
    header->frameNum = n - metadataPageNum;

    memory_memset(header->units, 0, header->frameNum * sizeof(FrameMetadata));
}

FrameMetadataUnit* frameMetadataHeader_getUnit(FrameMetadataHeader* header, void* frame) {
    Index32 index = FRAME_METADATA_FRAME_TO_INDEX(frame) - header->frameBaseIndex;
    DEBUG_ASSERT_SILENT(index < header->frameNum);
    return &header->units[index];
}

FrameMetadataUnit* frameMetadataHeader_getUnitFromIndex(FrameMetadataHeader* header, Index32 index) {
    Index32 indexInRegion = index - header->frameBaseIndex;
    DEBUG_ASSERT_SILENT(indexInRegion < header->frameNum);
    return &header->units[indexInRegion];
}

void frameMetadata_initStruct(FrameMetadata* metadata) {
    linkedList_initStruct(&metadata->headerList);
    metadata->frameNum = 0;
    metadata->lastAccessed = NULL;
}

FrameMetadataHeader* frameMetadata_addFrames(FrameMetadata* metadata, void* frames, Size n) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(frames));
    

    Index32 newFramesBeginIndex = FRAME_METADATA_FRAME_TO_INDEX(frames);
    LinkedListNode* insertBefore = metadata->headerList.next;
    for (; insertBefore != &metadata->headerList; insertBefore = insertBefore->next) {
        FrameMetadataHeader* header = HOST_POINTER(insertBefore, FrameMetadataHeader, node);

        DEBUG_ASSERT_SILENT(
            !RANGE_HAS_OVERLAP(
                newFramesBeginIndex, newFramesBeginIndex + n,
                header->frameBaseIndex, header->frameBaseIndex + header->frameNum
            )
        );

        if (header->frameBaseIndex > newFramesBeginIndex) {
            break;
        }
    }

    FrameMetadataHeader* newHeader = PAGING_CONVERT_KERNEL_MEMORY_P2V(frames);
    frameMetadataHeader_initStruct(newHeader, frames, n);
    linkedListNode_insertFront(insertBefore, &newHeader->node);

    metadata->frameNum += newHeader->frameNum;
    return newHeader;
}

FrameMetadataHeader* frameMetadata_getHeader(FrameMetadata* metadata, void* frame) {
    FrameMetadataHeader* lastAccessed = metadata->lastAccessed;
    if (lastAccessed != NULL && frameMetadataHeader_checkContain(lastAccessed, frame)) {
        return lastAccessed;
    }
    
    for (LinkedListNode* node = metadata->headerList.next; node != &metadata->headerList; node = node->next) {
        FrameMetadataHeader* header = HOST_POINTER(node, FrameMetadataHeader, node);
        if (frameMetadataHeader_checkContain(header, frame)) {
            metadata->lastAccessed = header;
            return header;
        }
    }

    ERROR_THROW_NO_GOTO(ERROR_ID_NOT_FOUND);
    return NULL;
}

FrameMetadataUnit* frameMetadata_getUnit(FrameMetadata* metadata, void* frame) {
    FrameMetadataHeader* header = frameMetadata_getHeader(&mm->frameMetadata, frame);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    return frameMetadataHeader_getUnit(header, frame);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void frameMetadata_assignToFrameAllocator(FrameMetadata* metadata, void* frames, Size n, FrameAllocator* allocator) {
    FrameMetadataHeader* header = frameMetadata_getHeader(metadata, frames);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (!frameMetadataHeader_checkRangeContain(header, frames, n)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    FrameMetadataUnit* unitBegin = frameMetadataHeader_getUnit(header, frames);
    for (int i = 0; i < n; ++i) {
        FrameMetadataUnit* unit = unitBegin + i;
        CLEAR_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR);
        SET_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR);
        unit->belongToAllocator = allocator;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    return;
}

void frameMetadata_assignToHeapAllocator(FrameMetadata* metadata, void* frames, Size n, HeapAllocator* allocator) {
    FrameMetadataHeader* header = frameMetadata_getHeader(metadata, frames);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (!frameMetadataHeader_checkRangeContain(header, frames, n)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    FrameMetadataUnit* unitBegin = frameMetadataHeader_getUnit(header, frames);
    for (int i = 0; i < n; ++i) {
        FrameMetadataUnit* unit = unitBegin + i;
        CLEAR_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR);
        SET_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR);
        unit->belongToAllocator = allocator;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    return;
}

void frameMetadata_clearAssignedAllocator(FrameMetadata* metadata, void* frames, Size n) {
    FrameMetadataHeader* header = frameMetadata_getHeader(metadata, frames);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (!frameMetadataHeader_checkRangeContain(header, frames, n)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    FrameMetadataUnit* unitBegin = frameMetadataHeader_getUnit(header, frames);
    for (int i = 0; i < n; ++i) {
        FrameMetadataUnit* unit = unitBegin + i;
        DEBUG_ASSERT_SILENT(TEST_FLAGS_CONTAIN(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR | FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR) && unit->belongToAllocator != NULL);
        CLEAR_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR | FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR);
        unit->belongToAllocator = NULL;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    return;
}

void frameMetadata_markCollected(FrameMetadata* metadata, void* frames, Size n) {
    FrameMetadataHeader* header = frameMetadata_getHeader(metadata, frames);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Index32 headIndex = FRAME_METADATA_FRAME_TO_INDEX(frames),
            endIndex = headIndex + n - 1;
    if (!frameMetadataHeader_checkRangeContainIndex(header, headIndex, n)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    FrameMetadataUnit* headUnit = frameMetadataHeader_getUnitFromIndex(header, headIndex);
    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(headUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE) && headUnit->collectedAnotherSideIndex == 0);
    SET_FLAG_BACK(headUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE);
    headUnit->collectedAnotherSideIndex = endIndex;
    
    if (n != 1) {
        FrameMetadataUnit* endUnit = frameMetadataHeader_getUnitFromIndex(header, endIndex);
        DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(endUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE) && endUnit->collectedAnotherSideIndex == 0);
        SET_FLAG_BACK(endUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE);
        endUnit->collectedAnotherSideIndex = headIndex;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    return;
}

void frameMetadata_unmarkCollected(FrameMetadata* metadata, void* frames, Size n) {
    FrameMetadataHeader* header = frameMetadata_getHeader(metadata, frames);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Index32 headIndex = FRAME_METADATA_FRAME_TO_INDEX(frames),
            endIndex = headIndex + n - 1;
    if (!frameMetadataHeader_checkRangeContainIndex(header, headIndex, n)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    FrameMetadataUnit* headUnit = frameMetadataHeader_getUnitFromIndex(header, headIndex);
    DEBUG_ASSERT_SILENT(TEST_FLAGS(headUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE) && headUnit->collectedAnotherSideIndex == endIndex);    //Must be begin side
    CLEAR_FLAG_BACK(headUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE);
    headUnit->collectedAnotherSideIndex = 0;
    
    if (n != 1) {
        FrameMetadataUnit* endUnit = frameMetadataHeader_getUnitFromIndex(header, endIndex);
        DEBUG_ASSERT_SILENT(TEST_FLAGS(endUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE) && endUnit->collectedAnotherSideIndex == headIndex);  //Must be end side
        CLEAR_FLAG_BACK(endUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE);
        endUnit->collectedAnotherSideIndex = 0;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    return;
}

Index32 frameMetadata_tryMergeNearbyCollected(FrameMetadata* metadata, void* sideFrame, bool direction) {
    FrameMetadataHeader* header = frameMetadata_getHeader(metadata, sideFrame);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Index32 index = FRAME_METADATA_FRAME_TO_INDEX(sideFrame);
    if (!frameMetadataHeader_checkContainIndex(header, index)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    FrameMetadataUnit* unit = frameMetadataHeader_getUnitFromIndex(header, index);
    DEBUG_ASSERT_SILENT(TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE) && unit->collectedAnotherSideIndex != 0 && frameMetadataHeader_checkContainIndex(header, unit->collectedAnotherSideIndex));
    FrameMetadataUnit* anotherSideUnit = frameMetadataHeader_getUnitFromIndex(header, unit->collectedAnotherSideIndex);

    Index32 nearbyFramesBeginIndex = INVALID_INDEX32;
    bool select = unit->collectedAnotherSideIndex == index ? direction : (unit->collectedAnotherSideIndex < index);
    if (select) { //It's end side
        Index32 nearbyIndex = index + 1;
        do {
            if (!frameMetadataHeader_checkContainIndex(header, nearbyIndex)) {
                break;
            }

            FrameMetadataUnit* nearbyUnit = frameMetadataHeader_getUnitFromIndex(header, nearbyIndex);
            if (TEST_FLAGS_FAIL(nearbyUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE) || unit->belongToAllocator != nearbyUnit->belongToAllocator) {
                break;
            }
            DEBUG_ASSERT_SILENT(nearbyUnit != NULL && nearbyUnit->collectedAnotherSideIndex >= nearbyIndex && frameMetadataHeader_checkContainIndex(header, nearbyUnit->collectedAnotherSideIndex));
            FrameMetadataUnit* nearbyAnotherSideUnit = frameMetadataHeader_getUnitFromIndex(header, nearbyUnit->collectedAnotherSideIndex);

            Index32 newBegin = unit->collectedAnotherSideIndex, newEnd = nearbyUnit->collectedAnotherSideIndex;
            unit->collectedAnotherSideIndex = nearbyUnit->collectedAnotherSideIndex = 0;
            
            anotherSideUnit->collectedAnotherSideIndex = newEnd;
            nearbyAnotherSideUnit->collectedAnotherSideIndex = newBegin;

            if (unit != anotherSideUnit) {
                CLEAR_FLAG(unit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE);
            }

            if (nearbyUnit != nearbyAnotherSideUnit) {
                CLEAR_FLAG(nearbyUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE);
            }
            nearbyFramesBeginIndex = nearbyIndex;
        } while (0);
    } else {    //It's begin side
        Index32 nearbyIndex = index - 1;
        do {
            if (!frameMetadataHeader_checkContainIndex(header, nearbyIndex)) {
                break;
            }

            FrameMetadataUnit* nearbyUnit = frameMetadataHeader_getUnitFromIndex(header, nearbyIndex);
            if (TEST_FLAGS_FAIL(nearbyUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE) || unit->belongToAllocator != nearbyUnit->belongToAllocator) {
                break;
            }
            DEBUG_ASSERT_SILENT(nearbyUnit != NULL && nearbyUnit->collectedAnotherSideIndex <= nearbyIndex && frameMetadataHeader_checkContainIndex(header, nearbyUnit->collectedAnotherSideIndex));
            FrameMetadataUnit* nearbyAnotherSideUnit = frameMetadataHeader_getUnitFromIndex(header, nearbyUnit->collectedAnotherSideIndex);

            Index32 newBegin = nearbyUnit->collectedAnotherSideIndex, newEnd = unit->collectedAnotherSideIndex;
            unit->collectedAnotherSideIndex = nearbyUnit->collectedAnotherSideIndex = 0;
            
            anotherSideUnit->collectedAnotherSideIndex = newBegin;
            nearbyAnotherSideUnit->collectedAnotherSideIndex = newEnd;

            if (unit != anotherSideUnit) {
                CLEAR_FLAG(unit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE);
            }

            if (nearbyUnit != nearbyAnotherSideUnit) {
                CLEAR_FLAG(nearbyUnit->flags, FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE);
            }
            nearbyFramesBeginIndex = nearbyUnit->collectedAnotherSideIndex;
        } while (0);
    }

    return nearbyFramesBeginIndex;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX32;
}