#include<fs/phospherus/blockLinkedList.h>

#include<fs/phospherus/phospherus.h>
#include<memory/buffer.h>

BlockLinkedListNode* __loadNode(BlockDevice* device, block_index_t nodeIndex, size_t offsetInBlock);

void __saveNode(BlockDevice* device, block_index_t nodeIndex, BlockLinkedListNode* node, size_t offsetInBlock);

void initBlockLinkedListNode(BlockLinkedListNode* node) {
    node->nextNodeIndex = PHOSPHERUS_NULL;
}

void blockLinkedListNodeSetNext(BlockLinkedListNode* node, block_index_t nextNodeIndex) {
    node->nextNodeIndex = nextNodeIndex;
}

block_index_t blockLinkedListNodeGetNext(BlockLinkedListNode* node) {
    return node->nextNodeIndex;
}

void blockLinkedListNodeRemoveNext(BlockDevice* device, BlockLinkedListNode* node, size_t offsetInBlock) {
    block_index_t nextNodeIndex = blockLinkedListNodeGetNext(node);

    BlockLinkedListNode* nextNode = __loadNode(device, nextNodeIndex, offsetInBlock);
    block_index_t nextNextNodeIndex = blockLinkedListNodeGetNext(nextNode);
    initBlockLinkedListNode(nextNode);
    __saveNode(device, nextNodeIndex, nextNode, offsetInBlock);

    blockLinkedListNodeSetNext(node, nextNextNodeIndex);
}

void blockLinkedListNodeInsertNext(BlockDevice* device, BlockLinkedListNode* node, block_index_t newNodeIndex, size_t offsetInBlock) {
    block_index_t nextNodeIndex = blockLinkedListNodeGetNext(node);
    blockLinkedListNodeSetNext(node, newNodeIndex);

    BlockLinkedListNode* newNode = __loadNode(device, newNodeIndex, offsetInBlock);
    blockLinkedListNodeSetNext(newNode, nextNodeIndex);
    __saveNode(device, newNodeIndex, newNode, offsetInBlock);
}

BlockLinkedListNode* __loadNode(BlockDevice* device, block_index_t nodeIndex, size_t offsetInBlock) {
    void* block = allocateBuffer(BUFFER_SIZE_512);
    device->readBlocks(device->additionalData, nodeIndex, block, 1);
    return (BlockLinkedListNode*)(block + offsetInBlock);
}

void __saveNode(BlockDevice* device, block_index_t nodeIndex, BlockLinkedListNode* node, size_t offsetInBlock) {
    void* block = ((void*)node) - offsetInBlock;
    device->writeBlocks(device->additionalData, nodeIndex, block, 1);
}