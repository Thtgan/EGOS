#if !defined(__MEMORY_FRAMEMETADATA_H)
#define __MEMORY_FRAMEMETADATA_H

typedef struct FrameMetadataUnit FrameMetadataUnit;
typedef struct FrameMetadataHeader FrameMetadataHeader;
typedef struct FrameMetadata FrameMetadata;

#include<debug.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocators/allocator.h>
#include<multitask/context.h>
#include<structs/linkedList.h>
#include<system/pageTable.h>

typedef struct FrameMetadataUnit {
    Flags8      flags;
#define FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR    FLAG16(0)
#define FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR   FLAG16(1)
#define FRAME_METADATA_UNIT_FLAGS_COLLECTED_REGION_SIDE     FLAG16(2)
    Uint8       reserved;
    Uint16      cow;
    union {
        Uint32  vRegionLength;
        Index32 collectedAnotherSideIndex;
    };
    void*       belongToAllocator;
} __attribute__((packed)) FrameMetadataUnit;

DEBUG_ASSERT_COMPILE(sizeof(FrameMetadataUnit) == 16);

typedef struct FrameMetadataHeader {
    Index32             frameBaseIndex;
    Size                frameNum;
    LinkedListNode      node;
    FrameMetadataUnit   units[0];
} FrameMetadataHeader;

#define FRAME_METADATA_FRAME_TO_INDEX(__FRAME)  ((Uintptr)(__FRAME) / PAGE_SIZE)

#define FRAME_METADATA_INDEX_TO_FRAME(__INDEX)  ((void*)((Uintptr)(__INDEX) * PAGE_SIZE))

void frameMetadataHeader_initStruct(FrameMetadataHeader* header, void* frames, Size n);

FrameMetadataUnit* frameMetadataHeader_getUnit(FrameMetadataHeader* header, Index32 frameIndex);

static inline void* frameMetadataHeader_getBase(FrameMetadataHeader* header) {
    return (void*)(((Uintptr)header->frameBaseIndex) * PAGE_SIZE);
}

static inline bool frameMetadataHeader_checkRangeContain(FrameMetadataHeader* header, Index32 frameIndex, Size n) {
    return RANGE_WITHIN(header->frameBaseIndex, header->frameBaseIndex + header->frameNum, frameIndex, frameIndex + n, <=, <=);
}

static inline bool frameMetadataHeader_checkContain(FrameMetadataHeader* header, Index32 frameIndex) {
    return VALUE_WITHIN(header->frameBaseIndex, header->frameBaseIndex + header->frameNum, frameIndex, <=, <);
}

typedef struct FrameMetadata {
    LinkedList              headerList;
    Size                    frameNum;
    FrameMetadataHeader*    lastAccessed;
} FrameMetadata;

void frameMetadata_initStruct(FrameMetadata* metadata);

FrameMetadataHeader* frameMetadata_addFrames(FrameMetadata* metadata, void* frames, Size n);

FrameMetadataHeader* frameMetadata_getHeader(FrameMetadata* metadata, Index32 frameIndex);

FrameMetadataUnit* frameMetadata_getUnit(FrameMetadata* metadata, Index32 frameIndex);

void frameMetadata_assignToFrameAllocator(FrameMetadata* metadata, Index32 framesBeginIndex, Size n, FrameAllocator* allocator);

void frameMetadata_assignToHeapAllocator(FrameMetadata* metadata, Index32 framesBeginIndex, Size n, HeapAllocator* allocator);

void frameMetadata_clearAssignedAllocator(FrameMetadata* metadata, Index32 framesBeginIndex, Size n);

void frameMetadata_markCollected(FrameMetadata* metadata, Index32 framesBeginIndex, Size n);

void frameMetadata_unmarkCollected(FrameMetadata* metadata, Index32 framesBeginIndex, Size n);

Index32 frameMetadata_tryMergeNearbyCollected(FrameMetadata* metadata, Index32 frameIndex, bool direction);

#endif // __MEMORY_FRAMEMETADATA_H
