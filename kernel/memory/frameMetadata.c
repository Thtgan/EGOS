#include<memory/frameMetadata.h>

#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocator.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<structs/linkedList.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<debug.h>
#include<error.h>
#include<kernel.h>

void frameMetadataUnit_markChunk(FrameMetadataUnit* unit, Size n) {
    SET_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_CHUNK_HEAD);
    unit->chunkLength = n;
}

Uint32 frameMetadataUnit_unmarkChunk(FrameMetadataUnit* unit) {
    Uint32 ret = unit->chunkLength;
    CLEAR_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_CHUNK_HEAD);
    unit->chunkLength = 0;

    return ret;
}

#define __FRAME_METADATA_PAGE_N(__N)    DIVIDE_ROUND_UP((sizeof(FrameMetadataHeader) + sizeof(FrameMetadata) * (__N)), PAGE_SIZE)
#define __FRAME_METADATA_BASE(__HEADER) ((FrameMetadataUnit*)((void*)(__HEADER) + sizeof(FrameMetadataHeader)))

void frameMetadataHeader_initStruct(FrameMetadataHeader* header, void* p, Size n) {
    Size metadataPageNum = __FRAME_METADATA_PAGE_N(n);
    if (metadataPageNum >= n) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    linkedListNode_initStruct(&header->node);
    header->frameBase = p + metadataPageNum * PAGE_SIZE;
    header->frameNum = n - metadataPageNum;

    return;
    ERROR_FINAL_BEGIN(0);
}

FrameMetadataUnit* frameMetadataHeader_getMetadataUnit(FrameMetadataHeader* header, void* p) {
    return __FRAME_METADATA_BASE(header) + DIVIDE_ROUND_DOWN(p - header->frameBase, PAGE_SIZE);
}

void frameMetadata_initStruct(FrameMetadata* metadata) {
    linkedList_initStruct(&metadata->headerList);
    metadata->frameNum = 0;
    metadata->lastAccessed = NULL;
}

FrameMetadataHeader* frameMetadata_addFrames(FrameMetadata* metadata, void* p, Size n) {
    FrameMetadataHeader* newHeader = paging_convertAddressP2V(p);

    LinkedListNode* insertBefore = metadata->headerList.next;
    for (; insertBefore != &metadata->headerList; insertBefore = insertBefore->next) {
        FrameMetadataHeader* header = HOST_POINTER(insertBefore, FrameMetadataHeader, node);

        DEBUG_ASSERT_SILENT(VALUE_WITHIN((Uintptr)header, (Uintptr)(header->frameBase + header->frameNum * PAGE_SIZE), (Uintptr)newHeader, <=, <));

        if ((Uintptr)header->frameBase > (Uintptr)newHeader->frameBase) {
            break;
        }
    }

    frameMetadataHeader_initStruct(newHeader, p, n);
    ERROR_GOTO_IF_ERROR(0);

    memory_memset(__FRAME_METADATA_BASE(newHeader), 0, newHeader->frameNum * sizeof(FrameMetadata));

    linkedListNode_insertFront(insertBefore, &newHeader->node);

    metadata->frameNum += newHeader->frameNum;
    return newHeader;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

FrameMetadataHeader* frameMetadata_getFrameMetadataHeader(FrameMetadata* metadata, void* p) {
    FrameMetadataHeader* lastAccessed = metadata->lastAccessed;
    if (lastAccessed != NULL && VALUE_WITHIN((Uintptr)lastAccessed->frameBase, (Uintptr)(lastAccessed->frameBase + lastAccessed->frameNum * PAGE_SIZE), (Uintptr)p, <=, <)) {
        return lastAccessed;
    }
    
    for (LinkedListNode* node = metadata->headerList.next; node != &metadata->headerList; node = node->next) {
        FrameMetadataHeader* header = HOST_POINTER(node, FrameMetadataHeader, node);
        if (!VALUE_WITHIN((Uintptr)header->frameBase, (Uintptr)(header->frameBase + header->frameNum * PAGE_SIZE), (Uintptr)p, <=, <)) {
            continue;
        }

        metadata->lastAccessed = header;

        return header;
    }

    ERROR_THROW_NO_GOTO(ERROR_ID_NOT_FOUND);
    return NULL;
}

FrameMetadataUnit* frameMetadata_getFrameMetadataUnit(FrameMetadata* metadata, void* p) {
    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, p);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    return frameMetadataHeader_getMetadataUnit(header, p);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}