#include<fs/phospherus/blockLinkedList.h>

#include<fs/phospherus/phospherus.h>
#include<kit/oop.h>
#include<memory/buffer.h>

Phospherus_BlockLinkedListNode* __loadNode(BlockDevice* device, block_index_t nodeIndex, size_t offsetInBlock);

void __saveNode(BlockDevice* device, block_index_t nodeIndex, Phospherus_BlockLinkedListNode* node, size_t offsetInBlock);

void phospherus_initBlockLinkedListNode(Phospherus_BlockLinkedListNode* node) {
    node->nextNodeIndex = PHOSPHERUS_NULL;
}

void phospherus_blockLinkedListNodeSetNext(Phospherus_BlockLinkedListNode* node, block_index_t nextNodeIndex) {
    node->nextNodeIndex = nextNodeIndex;
}

block_index_t phospherus_blockLinkedListNodeGetNext(Phospherus_BlockLinkedListNode* node) {
    return node->nextNodeIndex;
}

void phospherus_blockLinkedListNodeRemoveNext(BlockDevice* device, Phospherus_BlockLinkedListNode* node, size_t offsetInBlock) {
    block_index_t nextNodeIndex = phospherus_blockLinkedListNodeGetNext(node);

    Phospherus_BlockLinkedListNode* nextNode = __loadNode(device, nextNodeIndex, offsetInBlock);
    block_index_t nextNextNodeIndex = phospherus_blockLinkedListNodeGetNext(nextNode);
    phospherus_initBlockLinkedListNode(nextNode);
    __saveNode(device, nextNodeIndex, nextNode, offsetInBlock);

    phospherus_blockLinkedListNodeSetNext(node, nextNextNodeIndex);
}

void phospherus_blockLinkedListNodeInsertNext(BlockDevice* device, Phospherus_BlockLinkedListNode* node, block_index_t newNodeIndex, size_t offsetInBlock) {
    block_index_t nextNodeIndex = phospherus_blockLinkedListNodeGetNext(node);
    phospherus_blockLinkedListNodeSetNext(node, newNodeIndex);

    Phospherus_BlockLinkedListNode* newNode = __loadNode(device, newNodeIndex, offsetInBlock);
    phospherus_blockLinkedListNodeSetNext(newNode, nextNodeIndex);
    __saveNode(device, newNodeIndex, newNode, offsetInBlock);
}

Phospherus_BlockLinkedListNode* __loadNode(BlockDevice* device, block_index_t nodeIndex, size_t offsetInBlock) {
    void* block = allocateBuffer(BUFFER_SIZE_512);
    THIS_ARG_APPEND_OPERATION_CALL(device, readBlocks, nodeIndex, block, 1);
    return (Phospherus_BlockLinkedListNode*)(block + offsetInBlock);
}

void __saveNode(BlockDevice* device, block_index_t nodeIndex, Phospherus_BlockLinkedListNode* node, size_t offsetInBlock) {
    void* block = ((void*)node) - offsetInBlock;
    THIS_ARG_APPEND_OPERATION_CALL(device, writeBlocks, nodeIndex, block, 1);
    releaseBuffer(block, BUFFER_SIZE_512);
}