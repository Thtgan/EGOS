#if !defined(__PHOSPHERUS_BLOCK_LINKED_LIST_H)
#define __PHOSPHERUS_BLOCK_LINKED_LIST_H

#include<devices/block/blockDevice.h>

typedef struct {
    block_index_t nextNodeIndex;
} __attribute__((packed)) Phospherus_BlockLinkedListNode;

/**
 * @brief Initialize a block linked list node
 * 
 * @param node Node to initialize
 */
void phospherus_initBlockLinkedListNode(Phospherus_BlockLinkedListNode* node);

/**
 * @brief Set next node of a block linked list node
 * 
 * @param node Node to set
 * @param nextNodeIndex Block index of next node
 */
void phospherus_blockLinkedListNodeSetNext(Phospherus_BlockLinkedListNode* node, block_index_t nextNodeIndex);

/**
 * @brief Get next node of a block linked list node
 * 
 * @param node Node to get
 * @return block_index_t Block index of next node
 */
block_index_t phospherus_blockLinkedListNodeGetNext(Phospherus_BlockLinkedListNode* node);

/**
 * @brief Remove the next node, next node's record on device will be modified
 * 
 * @param device Device contain the node
 * @param node Node going to remove its next node
 * @param offsetInBlock Where is the node inside the block
 */
void phospherus_blockLinkedListNodeRemoveNext(BlockDevice* device, Phospherus_BlockLinkedListNode* node, size_t offsetInBlock);

/**
 * @brief Insert the next node, next node's record on device will be modified
 * 
 * @param device Device contain the node
 * @param node Node going to insert a new next node
 * @param newNodeIndex Block index of new next node
 * @param offsetInBlock Where is the node inside the block
 */
void phospherus_blockLinkedListNodeInsertNext(BlockDevice* device, Phospherus_BlockLinkedListNode* node, block_index_t newNodeIndex, size_t offsetInBlock);

#endif // __PHOSPHERUS_BLOCK_LINKED_LIST_H
