#if !defined(__PHOSPHERUS_BLOCK_LINKED_LIST_H)
#define __PHOSPHERUS_BLOCK_LINKED_LIST_H

#include<fs/blockDevice/blockDevice.h>

typedef struct {
    block_index_t thisNodeIndex;
    block_index_t nextNodeIndex;
    block_index_t prevNodeIndex;
} __attribute__((packed)) BlockLinkedListNode;

void initBlockLinkedListNode(BlockLinkedListNode* node, block_index_t nodeIndex);

void blockLinkedListNodeSetNext(BlockLinkedListNode* node, block_index_t nextNodeIndex);

block_index_t blockLinkedListNodeGetNext(BlockLinkedListNode* node);

void blockLinkedListNodeSetPrev(BlockLinkedListNode* node, block_index_t prevNodeIndex);

block_index_t blockLinkedListNodeGetPrev(BlockLinkedListNode* node);

void blockLinkedListNodeSetThis(BlockLinkedListNode* node, block_index_t thisNodeIndex);

block_index_t blockLinkedListNodeGetThis(BlockLinkedListNode* node);

void blockLinkedListNodeRemoveNext(BlockDevice* device, BlockLinkedListNode* node, size_t offsetInBlock);

void blockLinkedListNodeRemove(BlockDevice* device, BlockLinkedListNode* node, size_t offsetInBlock);

void blockLinkedListNodeInsertNext(BlockDevice* device, BlockLinkedListNode* node, block_index_t newNodeIndex, size_t offsetInBlock);

#endif // __PHOSPHERUS_BLOCK_LINKED_LIST_H
