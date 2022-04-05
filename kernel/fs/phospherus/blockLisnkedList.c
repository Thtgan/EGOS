#include<fs/phospherus/blockLinkedList.h>

#include<fs/phospherus/phospherus.h>

static char _buffer[512];//TODO: Replace with a reserved buffer

BlockLinkedListNode* __loadNode(BlockDevice* device, block_index_t nodeIndex, size_t offsetInBlock);

void __saveNode(BlockDevice* device, block_index_t nodeIndex);

void initBlockLinkedListNode(BlockLinkedListNode* node, block_index_t nodeIndex) {
    node->nextNodeIndex = node->prevNodeIndex = PHOSPHERUS_NULL;
    node->thisNodeIndex = nodeIndex;
}

void blockLinkedListNodeSetNext(BlockLinkedListNode* node, block_index_t nextNodeIndex) {
    node->nextNodeIndex = nextNodeIndex;
}

block_index_t blockLinkedListNodeGetNext(BlockLinkedListNode* node) {
    return node->nextNodeIndex;
}

void blockLinkedListNodeSetPrev(BlockLinkedListNode* node, block_index_t prevNodeIndex) {
    node->prevNodeIndex = prevNodeIndex;
}

block_index_t blockLinkedListNodeGetPrev(BlockLinkedListNode* node) {
    return node->prevNodeIndex;
}

void blockLinkedListNodeSetThis(BlockLinkedListNode* node, block_index_t thisNodeIndex) {
    node->thisNodeIndex = thisNodeIndex;
}

block_index_t blockLinkedListNodeGetThis(BlockLinkedListNode* node) {
    return node->thisNodeIndex;
}

void blockLinkedListNodeRemoveNext(BlockDevice* device, BlockLinkedListNode* node, size_t offsetInBlock) {
    block_index_t nextNodeIndex = blockLinkedListNodeGetNext(node);

    BlockLinkedListNode* nextNode = __loadNode(device, nextNodeIndex, offsetInBlock);
    block_index_t nextNextNodeIndex = blockLinkedListNodeGetNext(nextNode);
    initBlockLinkedListNode(nextNode, nextNodeIndex);
    __saveNode(device, nextNodeIndex);

    if (nextNextNodeIndex == PHOSPHERUS_NULL) {
        node->nextNodeIndex = PHOSPHERUS_NULL;
    } else {
        BlockLinkedListNode* nextNextNode = __loadNode(device, nextNextNodeIndex, offsetInBlock);
        blockLinkedListNodeSetNext(node, nextNextNodeIndex);
        block_index_t thisNodeIndex = blockLinkedListNodeGetThis(node);
        blockLinkedListNodeSetPrev(nextNextNode, thisNodeIndex);
        __saveNode(device, nextNextNodeIndex);
    }
}

void blockLinkedListNodeRemove(BlockDevice* device, BlockLinkedListNode* node, size_t offsetInBlock) {
    block_index_t nextNodeIndex = blockLinkedListNodeGetNext(node), prevNodeIndex = blockLinkedListNodeGetPrev(node), thisNodeIndex = blockLinkedListNodeGetThis(node);
    initBlockLinkedListNode(node, thisNodeIndex);

    if (prevNodeIndex != PHOSPHERUS_NULL) {
        BlockLinkedListNode* prevNode = __loadNode(device, prevNodeIndex, offsetInBlock);
        blockLinkedListNodeSetNext(prevNode, nextNodeIndex);
        __saveNode(device, prevNodeIndex);
    }

    if (nextNodeIndex != PHOSPHERUS_NULL) {
        BlockLinkedListNode* nextNode = __loadNode(device, nextNodeIndex, offsetInBlock);
        blockLinkedListNodeSetPrev(nextNode, prevNodeIndex);
        __saveNode(device, nextNodeIndex);
    }
}

#include<stdio.h>

void blockLinkedListNodeInsertNext(BlockDevice* device, BlockLinkedListNode* node, block_index_t newNodeIndex, size_t offsetInBlock) {
    block_index_t nextNodeIndex = blockLinkedListNodeGetNext(node), thisNodeIndex = blockLinkedListNodeGetThis(node);
    blockLinkedListNodeSetNext(node, newNodeIndex);

    if (nextNodeIndex != PHOSPHERUS_NULL) {
        BlockLinkedListNode* nextNode = __loadNode(device, nextNodeIndex, offsetInBlock);
        block_index_t thisNodeIndex = blockLinkedListNodeGetPrev(nextNode);
        blockLinkedListNodeSetPrev(nextNode, newNodeIndex);
        __saveNode(device, nextNodeIndex);
    }

    BlockLinkedListNode* newNode = __loadNode(device, newNodeIndex, offsetInBlock);
    blockLinkedListNodeSetNext(newNode, nextNodeIndex);
    blockLinkedListNodeSetPrev(newNode, thisNodeIndex);
    __saveNode(device, newNodeIndex);
}

BlockLinkedListNode* __loadNode(BlockDevice* device, block_index_t nodeIndex, size_t offsetInBlock) {
    device->readBlocks(device->additionalData, nodeIndex, _buffer, 1);
    return (BlockLinkedListNode*)(_buffer + offsetInBlock);
}

void __saveNode(BlockDevice* device, block_index_t nodeIndex) {
    device->writeBlocks(device->additionalData, nodeIndex, _buffer, 1);
}