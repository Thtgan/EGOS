#if !defined(__PHOSPHERUS_BLOCK_LINKED_LIST_H)
#define __PHOSPHERUS_BLOCK_LINKED_LIST_H

#include<fs/blockDevice/blockDevice.h>

typedef struct {
    block_index_t nextNodeIndex;
} __attribute__((packed)) BlockLinkedListNode;

void initBlockLinkedListNode(BlockLinkedListNode* node);

void blockLinkedListNodeSetNext(BlockLinkedListNode* node, block_index_t nextNodeIndex);

block_index_t blockLinkedListNodeGetNext(BlockLinkedListNode* node);

void blockLinkedListNodeRemoveNext(BlockDevice* device, BlockLinkedListNode* node, size_t offsetInBlock);

void blockLinkedListNodeInsertNext(BlockDevice* device, BlockLinkedListNode* node, block_index_t newNodeIndex, size_t offsetInBlock);

#endif // __PHOSPHERUS_BLOCK_LINKED_LIST_H
