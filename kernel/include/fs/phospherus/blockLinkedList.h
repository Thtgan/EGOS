#if !defined(__PHOSPHERUS_BLOCK_LINKED_LIST_H)
#define __PHOSPHERUS_BLOCK_LINKED_LIST_H

#include<fs/blockDevice/blockDevice.h>

typedef struct {
    block_index_t nextNodeIndex;
} __attribute__((packed)) Phospherus_BlockLinkedListNode;

void phospherus_initBlockLinkedListNode(Phospherus_BlockLinkedListNode* node);

void phospherus_blockLinkedListNodeSetNext(Phospherus_BlockLinkedListNode* node, block_index_t nextNodeIndex);

block_index_t phospherus_blockLinkedListNodeGetNext(Phospherus_BlockLinkedListNode* node);

void phospherus_blockLinkedListNodeRemoveNext(BlockDevice* device, Phospherus_BlockLinkedListNode* node, size_t offsetInBlock);

void phospherus_blockLinkedListNodeInsertNext(BlockDevice* device, Phospherus_BlockLinkedListNode* node, block_index_t newNodeIndex, size_t offsetInBlock);

#endif // __PHOSPHERUS_BLOCK_LINKED_LIST_H
