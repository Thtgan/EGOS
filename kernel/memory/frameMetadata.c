#include<memory/frameMetadata.h>

#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocator.h>
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
    header->frameBase = frames + metadataPageNum * PAGE_SIZE;
    header->frameNum = n - metadataPageNum;
}

FrameMetadataUnit* frameMetadataHeader_getMetadataUnit(FrameMetadataHeader* header, void* frame) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(frame));
    Index64 index = DIVIDE_ROUND_DOWN(frame - header->frameBase, PAGE_SIZE);
    DEBUG_ASSERT_SILENT(index < header->frameNum);
    return &header->units[index];
}

void frameMetadata_initStruct(FrameMetadata* metadata) {
    linkedList_initStruct(&metadata->headerList);
    metadata->frameNum = 0;
    metadata->lastAccessed = NULL;
}

FrameMetadataHeader* frameMetadata_addFrames(FrameMetadata* metadata, void* frames, Size n) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(frames));
    FrameMetadataHeader* newHeader = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(frames);

    LinkedListNode* insertBefore = metadata->headerList.next;
    for (; insertBefore != &metadata->headerList; insertBefore = insertBefore->next) {
        FrameMetadataHeader* header = HOST_POINTER(insertBefore, FrameMetadataHeader, node);

        DEBUG_ASSERT_SILENT(
            VALUE_WITHIN(
                (Uintptr)header, (Uintptr)(header->frameBase + header->frameNum * PAGE_SIZE),
                (Uintptr)newHeader,
                <=, <
            )
        );

        if ((Uintptr)header->frameBase > (Uintptr)newHeader->frameBase) {
            break;
        }
    }

    frameMetadataHeader_initStruct(newHeader, frames, n);

    memory_memset(newHeader->units, 0, newHeader->frameNum * sizeof(FrameMetadata));

    linkedListNode_insertFront(insertBefore, &newHeader->node);

    metadata->frameNum += newHeader->frameNum;
    return newHeader;
}

FrameMetadataHeader* frameMetadata_getFrameMetadataHeader(FrameMetadata* metadata, void* frame) {
    FrameMetadataHeader* lastAccessed = metadata->lastAccessed;
    if (lastAccessed != NULL && VALUE_WITHIN((Uintptr)lastAccessed->frameBase, (Uintptr)(lastAccessed->frameBase + lastAccessed->frameNum * PAGE_SIZE), (Uintptr)frame, <=, <)) {
        return lastAccessed;
    }
    
    for (LinkedListNode* node = metadata->headerList.next; node != &metadata->headerList; node = node->next) {
        FrameMetadataHeader* header = HOST_POINTER(node, FrameMetadataHeader, node);
        if (
            !VALUE_WITHIN(
                (Uintptr)header->frameBase, (Uintptr)(header->frameBase + header->frameNum * PAGE_SIZE),
                (Uintptr)frame,
                <=, <
            )
        ) {
            continue;
        }

        metadata->lastAccessed = header;

        return header;
    }

    ERROR_THROW_NO_GOTO(ERROR_ID_NOT_FOUND);
    return NULL;
}

FrameMetadataUnit* frameMetadata_getFrameMetadataUnit(FrameMetadata* metadata, void* frame) {
    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, frame);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    return frameMetadataHeader_getMetadataUnit(header, frame);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}