#if !defined(__MEMORY_FRAMEMETADATA_H)
#define __MEMORY_FRAMEMETADATA_H

typedef struct FrameMetadataUnit FrameMetadataUnit;
typedef struct FrameMetadataHeader FrameMetadataHeader;
typedef struct FrameMetadata FrameMetadata;

#include<debug.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<multitask/context.h>
#include<structs/linkedList.h>
#include<system/pageTable.h>

typedef struct FrameMetadataUnit {
    Flags8 flags;
#define FRAME_METADATA_UNIT_FLAGS_ALLOCATED  FLAG16(0)
#define FRAME_METADATA_UNIT_FLAGS_PAGE_TABLE FLAG16(1)
#define FRAME_METADATA_UNIT_FLAGS_CHUNK_HEAD FLAG16(2)
    Uint8 reserved;
    Uint16 cow;
    Uint32 chunkLength;
} __attribute__((packed)) FrameMetadataUnit;

DEBUG_ASSERT_COMPILE(sizeof(FrameMetadataUnit) == 8);

void frameMetadataUnit_markChunk(FrameMetadataUnit* unit, Size n);

Uint32 frameMetadataUnit_unmarkChunk(FrameMetadataUnit* unit);

typedef struct FrameMetadataHeader {
    void* frameBase;
    Size frameNum;
    LinkedListNode node;
} FrameMetadataHeader;

Result frameMetadataHeader_initStruct(FrameMetadataHeader* header, void* p, Size n);

FrameMetadataUnit* frameMetadataHeader_getMetadataUnit(FrameMetadataHeader* header, void* p);

typedef struct FrameMetadata {
    LinkedList headerList;
    Size frameNum;
    FrameMetadataHeader* lastAccessed;
} FrameMetadata;

void frameMetadata_initStruct(FrameMetadata* metadata);

FrameMetadataHeader* frameMetadata_addFrames(FrameMetadata* metadata, void* p, Size n);

FrameMetadataHeader* frameMetadata_getFrameMetadataHeader(FrameMetadata* metadata, void* p);

FrameMetadataUnit* frameMetadata_getFrameMetadataUnit(FrameMetadata* metadata, void* p);

#endif // __MEMORY_FRAMEMETADATA_H
