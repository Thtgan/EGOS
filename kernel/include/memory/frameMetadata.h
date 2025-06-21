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
    Flags8  flags;
#define FRAME_METADATA_UNIT_FLAGS_IS_REGION_HEAD        FLAG16(0)
#define FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP          FLAG16(1)
    Uint8   reserved;
    Uint16  cow;
    Uint32  vRegionLength;
} __attribute__((packed)) FrameMetadataUnit;

DEBUG_ASSERT_COMPILE(sizeof(FrameMetadataUnit) == 8);

typedef struct FrameMetadataHeader {
    void*               frameBase;
    Size                frameNum;
    LinkedListNode      node;
    FrameMetadataUnit   units[0];
} FrameMetadataHeader;

void frameMetadataHeader_initStruct(FrameMetadataHeader* header, void* frames, Size n);

FrameMetadataUnit* frameMetadataHeader_getMetadataUnit(FrameMetadataHeader* header, void* frame);

typedef struct FrameMetadata {
    LinkedList              headerList;
    Size                    frameNum;
    FrameMetadataHeader*    lastAccessed;
} FrameMetadata;

void frameMetadata_initStruct(FrameMetadata* metadata);

FrameMetadataHeader* frameMetadata_addFrames(FrameMetadata* metadata, void* frames, Size n);

FrameMetadataHeader* frameMetadata_getFrameMetadataHeader(FrameMetadata* metadata, void* frame);

FrameMetadataUnit* frameMetadata_getFrameMetadataUnit(FrameMetadata* metadata, void* frame);

#endif // __MEMORY_FRAMEMETADATA_H
