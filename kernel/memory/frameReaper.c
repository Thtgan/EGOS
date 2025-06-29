#include<memory/frameReaper.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/frameMetadata.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<structs/linkedList.h>

typedef struct __FrameReaperNode {
    LinkedListNode node;
    Size length;
} __FrameReaperNode;

static inline void __frameReaperNode_initStruct(__FrameReaperNode* node, Size n) {
    linkedListNode_initStruct(&node->node);
    node->length = n;
}

static inline void __frameReaperNode_merge(__FrameReaperNode* node1, __FrameReaperNode* node2) {
    DEBUG_ASSERT_SILENT((Uintptr)node1 + node1->length * PAGE_SIZE == (Uintptr)node2);

    node1->length += node2->length;
    linkedListNode_delete(&node2->node);
}

void __frameReaperNode_reap(__FrameReaperNode* node);

static inline void __frameReaper_initStruct(FrameReaper* reaper) {
    linkedList_initStruct(reaper);
}

void frameReaper_collect(FrameReaper* reaper, void* frames, Size n) {
    Index32 framesBeginIndex = FRAME_METADATA_FRAME_TO_INDEX(frames);
    frameMetadata_markCollected(&mm->frameMetadata, framesBeginIndex, n);
    __FrameReaperNode* mergedNode = (__FrameReaperNode*)PAGING_CONVERT_KERNEL_MEMORY_P2V(frames);
    __frameReaperNode_initStruct(mergedNode, n);
    linkedListNode_insertBack(reaper, &mergedNode->node);

    Index32 mergedFramesBeginIndex = INVALID_INDEX32;
    if ((mergedFramesBeginIndex = frameMetadata_tryMergeNearbyCollected(&mm->frameMetadata, framesBeginIndex, 0)) != INVALID_INDEX32) {
        __FrameReaperNode* previousNode = (__FrameReaperNode*)FRAME_METADATA_INDEX_TO_FRAME(mergedFramesBeginIndex);
        previousNode = (__FrameReaperNode*)PAGING_CONVERT_KERNEL_MEMORY_P2V(previousNode);
        __frameReaperNode_merge(previousNode, mergedNode);
        mergedNode = previousNode;
    }

    if ((mergedFramesBeginIndex = frameMetadata_tryMergeNearbyCollected(&mm->frameMetadata, framesBeginIndex + n - 1, 1)) != INVALID_INDEX32) {
        __FrameReaperNode* nextNode = (__FrameReaperNode*)FRAME_METADATA_INDEX_TO_FRAME(mergedFramesBeginIndex);
        nextNode = (__FrameReaperNode*)PAGING_CONVERT_KERNEL_MEMORY_P2V(nextNode);
        __frameReaperNode_merge(mergedNode, nextNode);
    }
}

void frameReaper_reap(FrameReaper* reaper) {
    for (LinkedListNode* node = linkedListNode_getNext(reaper); node != reaper;) {
        LinkedListNode* next = node->next;
        __FrameReaperNode* reaperNode = HOST_POINTER(node, __FrameReaperNode, node);
        __frameReaperNode_reap(reaperNode);
        node = next;
    }
}

void __frameReaperNode_reap(__FrameReaperNode* node) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(node));
    linkedListNode_delete(&node->node);
    Size n = node->length;
    void* frames = PAGING_CONVERT_KERNEL_MEMORY_V2P(node);
    Index32 framesIndex = FRAME_METADATA_FRAME_TO_INDEX(frames);

    frameMetadata_unmarkCollected(&mm->frameMetadata, framesIndex, n);

    FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, framesIndex);
    FrameAllocator* allocator = NULL;
    DEBUG_ASSERT_SILENT(TEST_FLAGS_CONTAIN(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR | FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR));
    if (TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR)) {
        allocator = ((HeapAllocator*)unit->belongToAllocator)->frameAllocator;
    } else {
        allocator = (FrameAllocator*)unit->belongToAllocator;
    }

    frameAllocator_freeFrames(allocator, frames, n);
}
