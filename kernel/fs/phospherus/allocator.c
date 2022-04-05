#include<fs/phospherus/allocator.h>

#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/blockLinkedList.h>
#include<fs/phospherus/phospherus.h>
#include<memory/malloc.h>
#include<memory/memory.h>
#include<stddef.h>
#include<stdint.h>
#include<system/systemInfo.h>

#define __CLUSTER_SIZE_IN_BLOCK 256
#define __CLUSTER_SIZE_IN_BYTE  (__CLUSTER_SIZE_IN_BLOCK * BLOCK_SIZE)

#define __CLUSTER_BLOCK_LIST_REMOVE_NEXT(__DEVICE, __CLUSTER_PTR, __LIST) blockLinkedListNodeRemoveNext(__DEVICE, &(__CLUSTER_PTR)->__LIST, offsetof(ClusterNode, __LIST))
#define __CLUSTER_BLOCK_LIST_REMOVE(__DEVICE, __CLUSTER_PTR, __LIST) blockLinkedListNodeRemove(__DEVICE, &(__CLUSTER_PTR)->__LIST, offsetof(ClusterNode, __LIST))
#define __CLUSTER_BLOCK_LIST_INSERT_NEXT(__DEVICE, __CLUSTER_PTR, __LIST, __NEW_NODE_INDEX) blockLinkedListNodeInsertNext(__DEVICE, &(__CLUSTER_PTR)->__LIST, __NEW_NODE_INDEX, offsetof(ClusterNode, __LIST))

static char _buffer[BLOCK_SIZE]; //TODO: Replace this with reserved buffer

void __initClusterNode(ClusterNode* clusterNode, block_index_t nodeIndex);

block_index_t __allocateFragmentBlock(block_index_t fragmentClusterIndex, ClusterNode* fragmentClusterNode);

void __releaseFragmentBlock(block_index_t fragmentClusterIndex, ClusterNode* fragmentClusterNode, block_index_t fragmentBlockIndex);

bool checkBlockDevice(const BlockDevice* device) {
    device->readBlocks(device->additionalData, 0, _buffer, 1);

    ClusterNode* superNode = (ClusterNode*)_buffer;
    bool ret = (superNode->signature == SYSTEM_INFO_MAGIC32);

    return ret;
}

//TODO: These read/write are ugly, simplify them
bool deployAllocator(const BlockDevice* device) {
    size_t blockNum = device->availableBlockNum;
    size_t clusterNum = blockNum / __CLUSTER_SIZE_IN_BLOCK;

    if (clusterNum < 1) {
        return false;
    }

    ClusterNode* node = (ClusterNode*)_buffer;
    __initClusterNode(node, 0);

    size_t freeClusterNum = clusterNum - 1, freeFragmentBlockNum = __CLUSTER_SIZE_IN_BLOCK - 1;
    for (int i = 0; i < clusterNum; ++i) {
        node->freeClusterNum = (i == 0) ? freeClusterNum : PHOSPHERUS_NULL;
        node->freeFragmentBlockNum = (i == 0) ? freeFragmentBlockNum : PHOSPHERUS_NULL;

        blockLinkedListNodeSetNext(&node->freeClusterListNode, i + 1 < clusterNum ? (i + 1) * __CLUSTER_SIZE_IN_BLOCK : PHOSPHERUS_NULL);
        blockLinkedListNodeSetPrev(&node->freeClusterListNode, i > 0 ? (i - 1) * __CLUSTER_SIZE_IN_BLOCK : PHOSPHERUS_NULL);
        blockLinkedListNodeSetThis(&node->freeClusterListNode, i * __CLUSTER_SIZE_IN_BLOCK);
        blockLinkedListNodeSetThis(&node->freeFragmentClusterListNode, i * __CLUSTER_SIZE_IN_BLOCK);

        device->writeBlocks(device->additionalData, i * __CLUSTER_SIZE_IN_BLOCK, node, 1);
    }

    return true;
}

void loadAllocator(Allocator* allocator, BlockDevice* device) {
    ClusterNode* superNode = malloc(sizeof(ClusterNode));

    device->readBlocks(device->additionalData, 0, superNode, 1);

    allocator->device = device;
    allocator->superNode = superNode;
}

void deleteAllocator(Allocator* allocator) {
    if (allocator == NULL || allocator->superNode == NULL) {
        return;
    }
    free(allocator->superNode);
    allocator->superNode = NULL;
}

block_index_t allocateCluster(Allocator* allocator) {
    BlockDevice* device = allocator->device;
    ClusterNode* superNode = allocator->superNode;
    if (superNode->freeClusterNum == 0) {
        return PHOSPHERUS_NULL;
    }

    block_index_t ret = blockLinkedListNodeGetNext(&superNode->freeClusterListNode);

    --superNode->freeClusterNum;
    __CLUSTER_BLOCK_LIST_REMOVE_NEXT(device, superNode, freeClusterListNode);

    device->writeBlocks(device->additionalData, 0, superNode, 1);

    return ret;
}

//TODO: Add the handle to multiple release to same cluster
void releaseCluster(Allocator* allocator, block_index_t clusterNodeIndex) {
    BlockDevice* device = allocator->device;
    ClusterNode* superNode = allocator->superNode;

    ClusterNode* node = (ClusterNode*)_buffer;
    __initClusterNode(node, clusterNodeIndex);

    device->writeBlocks(device->additionalData, clusterNodeIndex, node, 1);
    __CLUSTER_BLOCK_LIST_INSERT_NEXT(device, superNode, freeClusterListNode, clusterNodeIndex);

    ++superNode->freeClusterNum;
    device->writeBlocks(device->additionalData, 0, superNode, 1);
}

block_index_t allocateFragmentBlock(Allocator* allocator) {
    BlockDevice* device = allocator->device;
    ClusterNode* superNode = allocator->superNode;

    if (superNode->freeFragmentBlockNum == 0) {
        block_index_t freeClusterIndex = allocateCluster(allocator);

        if (freeClusterIndex == PHOSPHERUS_NULL) {
            return PHOSPHERUS_NULL;
        }

        superNode->freeFragmentBlockNum += (__CLUSTER_SIZE_IN_BLOCK - 1);
        __CLUSTER_BLOCK_LIST_INSERT_NEXT(device, superNode, freeFragmentClusterListNode, freeClusterIndex);
    }

    block_index_t fragmentClusterIndex = blockLinkedListNodeGetNext(&superNode->freeFragmentClusterListNode);

    block_index_t ret = PHOSPHERUS_NULL;
    if (fragmentClusterIndex != PHOSPHERUS_NULL) {
        device->readBlocks(device->additionalData, fragmentClusterIndex, _buffer, 1);
        ClusterNode* fragmentClusterNode = (ClusterNode*)_buffer;

        ret = __allocateFragmentBlock(fragmentClusterIndex, fragmentClusterNode);
        uint8_t freeBlockNumInCluster = fragmentClusterNode->freeBlockNumInCluster;
        device->writeBlocks(device->additionalData, fragmentClusterIndex, _buffer, 1);

        if (freeBlockNumInCluster == 0) {
            __CLUSTER_BLOCK_LIST_REMOVE_NEXT(device, superNode, freeFragmentClusterListNode);
        }
    } else {
        ret = __allocateFragmentBlock(0, superNode);
    }
    
    --superNode->freeFragmentBlockNum;

    device->writeBlocks(device->additionalData, 0, superNode, 1);

    return ret;
}

//TODO: Add the handle to multiple release to same block
void releaseFragmentBlock(Allocator* allocator, block_index_t fragmentBlockIndex) {
    BlockDevice* device = allocator->device;
    ClusterNode* superNode = allocator->superNode;

    block_index_t fragmentClusterIndex = (fragmentBlockIndex / __CLUSTER_SIZE_IN_BLOCK) * __CLUSTER_SIZE_IN_BLOCK;

    ++superNode->freeFragmentBlockNum;

    if (fragmentClusterIndex == 0) {
        __releaseFragmentBlock(0, superNode, fragmentBlockIndex);
    } else {
        device->readBlocks(device->additionalData, fragmentClusterIndex, _buffer, 1);

        ClusterNode* fragmentClusterNode = (ClusterNode*)_buffer;

        __releaseFragmentBlock(fragmentClusterIndex, fragmentClusterNode, fragmentBlockIndex);

        if (fragmentClusterNode->freeBlockNumInCluster == 1) {
            device->writeBlocks(device->additionalData, fragmentClusterIndex, _buffer, 1);

            __CLUSTER_BLOCK_LIST_INSERT_NEXT(device, superNode, freeFragmentClusterListNode, fragmentClusterIndex);
        } else if (fragmentClusterNode->freeBlockNumInCluster == (__CLUSTER_SIZE_IN_BLOCK - 1)) {
            if (fragmentClusterNode->freeFragmentClusterListNode.prevNodeIndex == 0) {
                device->writeBlocks(device->additionalData, fragmentClusterIndex, _buffer, 1);
                __CLUSTER_BLOCK_LIST_REMOVE_NEXT(device, superNode, freeFragmentClusterListNode);
            } else {
                __CLUSTER_BLOCK_LIST_REMOVE(device, fragmentClusterNode, freeFragmentClusterListNode);
            }

            superNode->freeFragmentBlockNum -= (__CLUSTER_SIZE_IN_BLOCK - 1);

            releaseCluster(allocator, fragmentClusterIndex);
        } else {
            device->writeBlocks(device->additionalData, fragmentClusterIndex, _buffer, 1);
        }
    }
    
    device->writeBlocks(device->additionalData, 0, superNode, 1);
}

void __initClusterNode(ClusterNode* clusterNode, block_index_t nodeIndex) {
    memset(clusterNode, 0, sizeof(ClusterNode));
    clusterNode->signature = SYSTEM_INFO_MAGIC32;

    clusterNode->freeClusterNum = clusterNode->freeFragmentBlockNum = PHOSPHERUS_NULL;
    initBlockLinkedListNode(&clusterNode->freeClusterListNode, nodeIndex);
    initBlockLinkedListNode(&clusterNode->freeFragmentClusterListNode, nodeIndex);

    clusterNode->freeBlockNumInCluster = __CLUSTER_SIZE_IN_BLOCK - 1;
    clusterNode->firstFreeBlock = 0;
    for (int i = 0; i < (__CLUSTER_SIZE_IN_BLOCK - 1); ++i) {
        clusterNode->freeBlockList[i] = i + 1;
    }
}

block_index_t __allocateFragmentBlock(block_index_t fragmentClusterIndex, ClusterNode* fragmentClusterNode) {
    uint8_t fragmentIndex = fragmentClusterNode->firstFreeBlock;

    fragmentClusterNode->firstFreeBlock = fragmentClusterNode->freeBlockList[fragmentIndex];
    fragmentClusterNode->freeBlockList[fragmentIndex] = PHOSPHERUS_NULL;
    --fragmentClusterNode->freeBlockNumInCluster;

    return fragmentClusterIndex + fragmentIndex + 1;
}

void __releaseFragmentBlock(block_index_t fragmentClusterIndex, ClusterNode* fragmentClusterNode, block_index_t fragmentBlockIndex) {
    uint8_t fragmentIndex = fragmentBlockIndex - fragmentClusterIndex - 1;
    fragmentClusterNode->freeBlockList[fragmentIndex] = fragmentClusterNode->firstFreeBlock;
    fragmentClusterNode->firstFreeBlock = fragmentIndex;
    ++fragmentClusterNode->freeBlockNumInCluster;
}