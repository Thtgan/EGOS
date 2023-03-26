#include<fs/phospherus/allocator.h>

#include<devices/block/blockDevice.h>
#include<fs/phospherus/phospherus.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<system/systemInfo.h>

#define __SUPER_NODE_CLUSTER_SIZE   256
#define __SUPER_NODE_SIZE           CLUSTER_BLOCK_SIZE
#define __SUPER_NODE_SPAN           (__SUPER_NODE_CLUSTER_SIZE * CLUSTER_BLOCK_SIZE)

/**
 * So this is how phospherus's allocator works:
 * This thing, now allocate clusters in 4KB (8 blocks), at the very beginning version it allocate blocks
 * Though I have never gone through the performance bottleneck, but it is obviously an improper design
 * The super nodes are spreaded over the device evenly, each super node takes over 1MB or 256 clusters
 * The node itself takes 1 cluster and it cannot evolve the allocation, so the 1/256 of the device is not available
 * Root super node maintains the whole allocator, but it also a normal super node and it allocates cluster as well
 * The nodes with any free clucters on the device forms a linked list, the root super node will record where is its head and tail
 * Allocate happens on the super node which is the head of the linked list, once it runs out of clusters, it will be removed from the list
 * and the root super node will record new super node with free clusters
 * When it comes to the releasing cluster, it is easy when the super node the cluster belongs to is in the linked list,
 * when it is not, revived super node will be attached to the end of the super node list, such a design avoids frequent allocating/releasing on a super node with few free clusters
 * Also, as said above, root node will maintain the information to make the allocator works, and be capable to recover from shutting down.
 */

typedef struct {
    uint64_t signature;

    size_t deviceFreeClusterNum;

    Index64 firstFreeSuperNode;

    Index64 lastFreeSuperNode;

    Index64 firstFreeBlockStack;
    
    Index64 lastFreeBlockStack;

    uint64_t reserved1[58];
} __attribute__((packed)) __DeviceInfo;

#define __DEVICE_INFO_SIZE          (sizeof(__DeviceInfo) / BLOCK_SIZE)

typedef struct {
    uint64_t signature1;    //Signature for validation

    Index64 blockIndex;

    Index64 nextFreeSuperNode;

    uint64_t reserved1[60];

    uint64_t signature2;    //Signature for validation
} __attribute__((packed)) __SuperNodeInfo;

#define __SUPER_NODE_INFO_SIZE      (sizeof(__SuperNodeInfo) / BLOCK_SIZE)

typedef struct {
    Index64 stackTop;
    Index64 clusterStack[__SUPER_NODE_CLUSTER_SIZE - 1];
} __attribute__((packed)) __ClusterStack;

#define CLUSTER_STACK_EMPTY(__CLUSTER_STACK)        ((__CLUSTER_STACK)->stackTop == -1)
#define POP_CLUSTER(__CLUSTER_STACK)                ((__CLUSTER_STACK)->clusterStack[(__CLUSTER_STACK)->stackTop--])
#define PUSH_CLUSTER(__CLUSTER_STACK, __CLUSTER)    ((__CLUSTER_STACK)->clusterStack[++(__CLUSTER_STACK)->stackTop] = (__CLUSTER))

#define __SUPER_NODE_STACK_OFFSET   (offsetof(__SuperNode, stack) / BLOCK_SIZE)
#define __SUPER_NODE_STACK_SIZE     (sizeof(__ClusterStack) / BLOCK_SIZE)

typedef struct {
    __SuperNodeInfo info;
    uint64_t reserved2[192];
    __ClusterStack stack;
} __attribute__((packed)) __SuperNode;

typedef struct {
    Index64 nextFreeBlockStack;
    Index64 blockIndex;

    uint64_t reserved[54];
    
    Index64 stackTop;
    Index64 blockStack[CLUSTER_BLOCK_SIZE - 1];
} __attribute__((packed)) __BlockStack;

#define BLOCK_STACK_EMPTY(__BLOCK_STACK)            ((__BLOCK_STACK)->stackTop == -1)
#define POP_BLOCK(__BLOCK_STACK)                    ((__BLOCK_STACK)->blockStack[(__BLOCK_STACK)->stackTop--])
#define PUSH_BLOCK(__BLOCK_STACK, __BLOCK)          ((__BLOCK_STACK)->blockStack[++(__BLOCK_STACK)->stackTop] = (__BLOCK))

#define __BLOCK_STACK_SIZE  (sizeof(__BlockStack) / BLOCK_SIZE)

static HashTable _hashTable;
static SinglyLinkedList _hashChains[32];

void phospherusInitAllocator() {
    initHashTable(&_hashTable, 32, _hashChains, LAMBDA(size_t, (HashTable* this, Object key) {
        return (size_t)key % 31;
    }));
}

bool phospherusCheckBlockDevice(BlockDevice* device) {
    __SuperNodeInfo* rootSuperNodeInfo = allocateBuffer(BUFFER_SIZE_512);
    blockDeviceReadBlocks(device, ROOT_SUPER_NODE_INDEX, rootSuperNodeInfo, __SUPER_NODE_INFO_SIZE);

    bool ret = (rootSuperNodeInfo->signature1 == SYSTEM_INFO_MAGIC64 && rootSuperNodeInfo->signature2 == SYSTEM_INFO_MAGIC64);   //Check by reading the signature

    releaseBuffer(rootSuperNodeInfo, BUFFER_SIZE_512);

    return ret;
}

bool phospherusDeployAllocator(BlockDevice* device) {
    size_t deviceSize = device->availableBlockNum;
    size_t nodeNum = deviceSize / __SUPER_NODE_SPAN;

    if (nodeNum < 1) {   //The block device must have at least one node length free (1MB)
        return false;
    }

    __DeviceInfo* info = allocateBuffer(BUFFER_SIZE_512);
    //First free cluster of root super node is reserved for special purpose like root directory
    info->signature = SYSTEM_INFO_MAGIC64;
    info->deviceFreeClusterNum = deviceSize / CLUSTER_BLOCK_SIZE - nodeNum - 1;
    info->firstFreeBlockStack = 0, info->firstFreeBlockStack = (nodeNum - 1) * __SUPER_NODE_SPAN;
    info->firstFreeBlockStack = info->lastFreeBlockStack = PHOSPHERUS_NULL;
    blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, info, __DEVICE_INFO_SIZE);
    releaseBuffer(info, BUFFER_SIZE_512);
    
    __SuperNode* node = allocateBuffer(BUFFER_SIZE_4096);
    memset(node, 0, sizeof(__SuperNode));

    //Public info
    __SuperNodeInfo* nodeInfo = &node->info;
    __ClusterStack* stack = &node->stack;
    nodeInfo->signature1 = nodeInfo->signature2 = SYSTEM_INFO_MAGIC64;

    for (int i = 0; i < nodeNum; ++i) {
        nodeInfo->blockIndex = i * __SUPER_NODE_SPAN;
        nodeInfo->nextFreeSuperNode = i + 1 == nodeNum ? PHOSPHERUS_NULL : (i + 1) * __SUPER_NODE_SPAN;

        if (i == ROOT_SUPER_NODE_INDEX) {
            stack->stackTop = 253;
            for (int j = 255; j > 1; --j) {
                stack->clusterStack[255 - j] = nodeInfo->blockIndex + j * CLUSTER_BLOCK_SIZE;
            }
        } else {
            stack->stackTop = 254;
            for (int j = 255; j >= 1; --j) {
                stack->clusterStack[255 - j] = nodeInfo->blockIndex + j * CLUSTER_BLOCK_SIZE;
            }
        }

        blockDeviceWriteBlocks(device, nodeInfo->blockIndex, node, __SUPER_NODE_SIZE);
    }

    releaseBuffer(node, BUFFER_SIZE_4096);

    return true;
}

PhospherusAllocator* phospherusOpenAllocator(BlockDevice* device) {
    PhospherusAllocator* ret = NULL;
    HashChainNode* node = NULL;
    if ((node = hashTableFind(&_hashTable, (Object)device->deviceID)) != NULL) { //If allocator opened, give a reference
        ret = HOST_POINTER(node, PhospherusAllocator, hashChainNode);
        ++ret->openCnt;
        return ret;
    }

    ret = kMalloc(sizeof(PhospherusAllocator), MEMORY_TYPE_NORMAL);
    __DeviceInfo* deviceInfo = kMalloc(sizeof(__DeviceInfo), MEMORY_TYPE_NORMAL);
    __SuperNodeInfo* firstFreeSuperNode = kMalloc(sizeof(__SuperNode), MEMORY_TYPE_NORMAL);
    __BlockStack * firstFreeBlockStack = kMalloc(sizeof(__BlockStack), MEMORY_TYPE_NORMAL);

    blockDeviceReadBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE);
    if (deviceInfo->firstFreeSuperNode != PHOSPHERUS_NULL) {
        blockDeviceReadBlocks(device, deviceInfo->firstFreeSuperNode, firstFreeSuperNode, __SUPER_NODE_SIZE);
    }
    
    if (deviceInfo->firstFreeBlockStack != PHOSPHERUS_NULL) {
        blockDeviceReadBlocks(device, deviceInfo->firstFreeBlockStack, firstFreeBlockStack, __BLOCK_STACK_SIZE);
    }

    ret->device = device;
    ret->deviceInfo = deviceInfo;
    ret->firstFreeSuperNode = firstFreeSuperNode;
    ret->firstFreeBlockStack = firstFreeBlockStack;
    ret->openCnt = 1;
    initHashChainNode(&ret->hashChainNode);

    hashTableInsert(&_hashTable, (Object)device->deviceID, &ret->hashChainNode);

    return ret;
}

void phospherusCloseAllocator(PhospherusAllocator* allocator) {
    if (--allocator->openCnt > 0) {
        return; //Something still using this, just return
    }

    BlockDevice* device = allocator->device;

    hashTableDelete(&_hashTable, (Object)device->deviceID);

    kFree(allocator->deviceInfo);
    kFree(allocator->firstFreeSuperNode);
    kFree(allocator->firstFreeBlockStack);
    allocator->deviceInfo = allocator->firstFreeSuperNode = allocator->firstFreeBlockStack = NULL;
    kFree(allocator);
}

Index64 phospherusAllocateCluster(PhospherusAllocator* allocator) {
    BlockDevice* device = allocator->device;
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    __SuperNode* firstFreeSuperNode = allocator->firstFreeSuperNode;

    if (deviceInfo->deviceFreeClusterNum == 0 || deviceInfo->firstFreeSuperNode == PHOSPHERUS_NULL) {    //Theoretically, these two can be true iff at the same time
        return PHOSPHERUS_NULL;
    }

    __ClusterStack* stack = &firstFreeSuperNode->stack;
    __SuperNodeInfo* info = &firstFreeSuperNode->info;
    Index64 ret = POP_CLUSTER(stack);   //Pop one cluster from super node in memory

    blockDeviceWriteBlocks(device, info->blockIndex + __SUPER_NODE_STACK_OFFSET, stack, __SUPER_NODE_STACK_SIZE);   //Save back the stack

    if (CLUSTER_STACK_EMPTY(stack)) {                                                                                                       //If the stack is empty now, replace with new free super node
        deviceInfo->firstFreeSuperNode = info->nextFreeSuperNode;                                                                           //Will be wrote in in updating
        if (info->nextFreeSuperNode == PHOSPHERUS_NULL) {
            memset(firstFreeSuperNode, 0, sizeof(__SuperNode));                                                                             //Destroy the invalid data
            deviceInfo->lastFreeSuperNode = PHOSPHERUS_NULL;
        } else {          
            blockDeviceReadBlocks(device, info->nextFreeSuperNode, firstFreeSuperNode, __SUPER_NODE_SIZE);  //Read new free super block, no need to modify old free super block's info
        }
    }

    --deviceInfo->deviceFreeClusterNum;  //Save root super node
    blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE);
    return ret;
}

void phospherusReleaseCluster(PhospherusAllocator* allocator, Index64 cluster) {
    if (cluster == PHOSPHERUS_NULL) {    //Well, just in case
        return;
    }

    BlockDevice* device = allocator->device;
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    __SuperNode* firstFreeSuperNode = allocator->firstFreeSuperNode;

    Index64 superNodeIndex = (cluster / __SUPER_NODE_SPAN) * __SUPER_NODE_SPAN;  //Which super node does this cluster belongs to
    if (superNodeIndex == firstFreeSuperNode->info.blockIndex) {
        PUSH_CLUSTER(&firstFreeSuperNode->stack, cluster);          //It belongs to first free super block, which is already in memory, just save in
        blockDeviceWriteBlocks(device, superNodeIndex + __SUPER_NODE_STACK_OFFSET, &firstFreeSuperNode->stack, __SUPER_NODE_STACK_SIZE);
    } else {    //It belongs to normal super node, not in memory
        __SuperNode* superNode = allocateBuffer(BUFFER_SIZE_4096);
        blockDeviceReadBlocks(device, superNodeIndex, superNode, __SUPER_NODE_SIZE);

        bool needInsert = CLUSTER_STACK_EMPTY(&superNode->stack);
        PUSH_CLUSTER(&superNode->stack, cluster);

        if (needInsert) {   //The release make a super block free, need to be inserted back to the list
            if (deviceInfo->lastFreeSuperNode == PHOSPHERUS_NULL) {  //When deivce has no free super node
                deviceInfo->firstFreeSuperNode = superNodeIndex;
            } else if (deviceInfo->lastFreeSuperNode == firstFreeSuperNode->info.blockIndex) {
                firstFreeSuperNode->info.nextFreeSuperNode = superNodeIndex;
                blockDeviceWriteBlocks(device, deviceInfo->lastFreeSuperNode, &firstFreeSuperNode->info, __SUPER_NODE_INFO_SIZE);
            } else {
                __SuperNodeInfo* lastFreeSuperNodeInfo = allocateBuffer(BUFFER_SIZE_512);  //Update linked list tail
                blockDeviceReadBlocks(device, deviceInfo->lastFreeSuperNode, lastFreeSuperNodeInfo, __SUPER_NODE_INFO_SIZE);
                lastFreeSuperNodeInfo->nextFreeSuperNode = superNodeIndex;
                blockDeviceWriteBlocks(device, deviceInfo->lastFreeSuperNode, lastFreeSuperNodeInfo, __SUPER_NODE_INFO_SIZE);
                releaseBuffer(lastFreeSuperNodeInfo, BUFFER_SIZE_512);
            }
            
            superNode->info.nextFreeSuperNode = PHOSPHERUS_NULL;    //Make tail a tail, and update the root super block
            deviceInfo->lastFreeSuperNode = superNodeIndex;
        }

        blockDeviceWriteBlocks(device, superNodeIndex, superNode, __SUPER_NODE_SIZE);
        releaseBuffer(superNode, BUFFER_SIZE_4096);
    }

    ++deviceInfo->deviceFreeClusterNum;  //Save root super node
    blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE);
}

size_t phospherusGetFreeClusterNum(PhospherusAllocator* allocator) {
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    return deviceInfo->deviceFreeClusterNum;
}

Index64 phospherusAllocateBlock(PhospherusAllocator* allocator) {
    BlockDevice* device = allocator->device;
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    __BlockStack* firstFreeBlockStack = allocator->firstFreeBlockStack;

    if (deviceInfo->firstFreeBlockStack == PHOSPHERUS_NULL) {
        Index64 cluster = phospherusAllocateCluster(allocator);

        if (cluster == PHOSPHERUS_NULL) {
            return PHOSPHERUS_NULL;
        }

        __BlockStack* blockStack = (__BlockStack*)allocateBuffer(BUFFER_SIZE_512);
        memset(blockStack, 0, sizeof(__BlockStack));

        blockStack->nextFreeBlockStack = PHOSPHERUS_NULL;
        blockStack->blockIndex = cluster;
        blockStack->stackTop = 6;
        for (int i = 0; i < CLUSTER_BLOCK_SIZE - 1; ++i) {
            blockStack->blockStack[i] = cluster + i + 1;
        }

        blockDeviceWriteBlocks(device, cluster, blockStack, __BLOCK_STACK_SIZE);

        memcpy(firstFreeBlockStack, blockStack, sizeof(__BlockStack));
        deviceInfo->firstFreeBlockStack = deviceInfo->lastFreeBlockStack = blockStack->blockIndex;
        blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE);

        releaseBuffer(blockStack, BUFFER_SIZE_512);
    }

    Index64 ret = POP_BLOCK(firstFreeBlockStack);
    blockDeviceWriteBlocks(device, firstFreeBlockStack->blockIndex, firstFreeBlockStack, __BLOCK_STACK_SIZE);

    if (BLOCK_STACK_EMPTY(firstFreeBlockStack)) {
        deviceInfo->firstFreeBlockStack = firstFreeBlockStack->nextFreeBlockStack;
        if (firstFreeBlockStack->nextFreeBlockStack == PHOSPHERUS_NULL) {
            deviceInfo->lastFreeBlockStack = PHOSPHERUS_NULL;
            memset(firstFreeBlockStack, 0, sizeof(__BlockStack));
        } else {
            blockDeviceReadBlocks(device, firstFreeBlockStack->nextFreeBlockStack, firstFreeBlockStack, __BLOCK_STACK_SIZE);
        }

        blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE);
    }

    return ret;
}

void phospherusReleaseBlock(PhospherusAllocator* allocator, Index64 block) {
    BlockDevice* device = allocator->device;
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    __BlockStack* firstFreeBlockStack = allocator->firstFreeBlockStack;

    Index64 blockStackIndex = (block / CLUSTER_BLOCK_SIZE) * CLUSTER_BLOCK_SIZE;

    if (blockStackIndex == deviceInfo->firstFreeBlockStack) {
        PUSH_BLOCK(firstFreeBlockStack, block);
        blockDeviceWriteBlocks(device, blockStackIndex, firstFreeBlockStack, __BLOCK_STACK_SIZE);
    } else {
        __BlockStack* stack = allocateBuffer(BUFFER_SIZE_512);
        blockDeviceReadBlocks(device, blockStackIndex, stack, __BLOCK_STACK_SIZE);
        bool needInsert = BLOCK_STACK_EMPTY(stack);
        PUSH_BLOCK(stack, block);

        if (needInsert) {
            if (deviceInfo->lastFreeBlockStack == PHOSPHERUS_NULL) {
                deviceInfo->firstFreeBlockStack = blockStackIndex;
                memcpy(firstFreeBlockStack, stack, sizeof(__BlockStack));
            } else if (deviceInfo->lastFreeBlockStack == firstFreeBlockStack->blockIndex) {
                firstFreeBlockStack->nextFreeBlockStack = blockStackIndex;
                blockDeviceWriteBlocks(device, deviceInfo->lastFreeBlockStack, firstFreeBlockStack, __BLOCK_STACK_SIZE);
            } else {
                __BlockStack* lastFreeBlockStack = allocateBuffer(BUFFER_SIZE_512);
                blockDeviceReadBlocks(device, deviceInfo->lastFreeBlockStack, lastFreeBlockStack, __BLOCK_STACK_SIZE);
                lastFreeBlockStack->nextFreeBlockStack = blockStackIndex;
                blockDeviceWriteBlocks(device, deviceInfo->lastFreeBlockStack, lastFreeBlockStack, __BLOCK_STACK_SIZE);
                releaseBuffer(lastFreeBlockStack, BUFFER_SIZE_512);
            }

            stack->nextFreeBlockStack = PHOSPHERUS_NULL;
            deviceInfo->lastFreeBlockStack = blockStackIndex;
        }

        blockDeviceWriteBlocks(device, blockStackIndex, stack, __BLOCK_STACK_SIZE);
        releaseBuffer(stack, BUFFER_SIZE_512);

        blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE);
    }
}