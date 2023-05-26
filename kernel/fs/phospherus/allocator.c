#include<fs/phospherus/allocator.h>

#include<devices/block/blockDevice.h>
#include<error.h>
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
    Uint64 signature;

    Size deviceFreeClusterNum;

    Index64 firstFreeSuperNode;

    Index64 lastFreeSuperNode;

    Index64 firstFreeBlockStack;
    
    Index64 lastFreeBlockStack;

    Uint64 reserved1[58];
} __attribute__((packed)) __DeviceInfo;

#define __DEVICE_INFO_SIZE          (sizeof(__DeviceInfo) / BLOCK_SIZE)

typedef struct {
    Uint64 signature1;    //Signature for validation

    Index64 blockIndex;

    Index64 nextFreeSuperNode;

    Uint64 reserved1[60];

    Uint64 signature2;    //Signature for validation
} __attribute__((packed)) __SuperNodeInfo;

#define __SUPER_NODE_INFO_OFFSET    (offsetof(__SuperNode, info) / BLOCK_SIZE)
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
    Uint64 reserved2[192];
    __ClusterStack stack;
} __attribute__((packed)) __SuperNode;

typedef struct {
    Index64 nextFreeBlockStack;
    Index64 blockIndex;

    Uint64 reserved[54];
    
    Index64 stackTop;
    Index64 blockStack[CLUSTER_BLOCK_SIZE - 1];
} __attribute__((packed)) __BlockStack;

#define BLOCK_STACK_EMPTY(__BLOCK_STACK)            ((__BLOCK_STACK)->stackTop == -1)
#define POP_BLOCK(__BLOCK_STACK)                    ((__BLOCK_STACK)->blockStack[(__BLOCK_STACK)->stackTop--])
#define PUSH_BLOCK(__BLOCK_STACK, __BLOCK)          ((__BLOCK_STACK)->blockStack[++(__BLOCK_STACK)->stackTop] = (__BLOCK))

#define __BLOCK_STACK_SIZE  (sizeof(__BlockStack) / BLOCK_SIZE)

static HashTable _hashTable;
static SinglyLinkedList _hashChains[32];

static Result __doDeployAllocator(BlockDevice* device, __DeviceInfo** infoPtr, __SuperNode** superNodePtr);

static Result __doReleaseCluster(PhospherusAllocator* allocator, Index64 cluster, void** belongToPtr, void** tailSuperNodeInfoPtr);

static Index64 __doAllocateBlock(PhospherusAllocator* allocator, void** blockStackPtr);

static Result __doReleaseBlock(PhospherusAllocator* allocator, Index64 block, void** belongToPtr, void** tailBlockStackPtr);

void phospherusInitAllocator() {
    initHashTable(&_hashTable, 32, _hashChains, LAMBDA(Size, (HashTable* this, Object key) {
        return (Size)key % 31;
    }));
}

Result phospherusCheckBlockDevice(BlockDevice* device) {
    __SuperNodeInfo* rootSuperNodeInfo = allocateBuffer(BUFFER_SIZE_512);

    if (rootSuperNodeInfo == NULL || blockDeviceReadBlocks(device, ROOT_SUPER_NODE_INDEX, rootSuperNodeInfo, __SUPER_NODE_INFO_SIZE) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (rootSuperNodeInfo->signature1 != SYSTEM_INFO_MAGIC64 || rootSuperNodeInfo->signature2 != SYSTEM_INFO_MAGIC64) {
        releaseBuffer(rootSuperNodeInfo, BUFFER_SIZE_512);
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    releaseBuffer(rootSuperNodeInfo, BUFFER_SIZE_512);

    return RESULT_SUCCESS;
}

Result phospherusDeployAllocator(BlockDevice* device) {
    __DeviceInfo* info = NULL;
    __SuperNode* superNode = NULL;

    Result ret = __doDeployAllocator(device, &info, &superNode);

    if (superNode != NULL) {
        releaseBuffer(superNode, BUFFER_SIZE_4096);
    }

    if (info != NULL) {
        releaseBuffer(info, BUFFER_SIZE_512);
    }

    return ret;
}

PhospherusAllocator* phospherusOpenAllocator(BlockDevice* device) {
    PhospherusAllocator* ret = NULL;

    HashChainNode* node = NULL;
    if ((node = hashTableFind(&_hashTable, (Object)device->deviceID)) != NULL) { //If allocator opened, give a reference
        ret = HOST_POINTER(node, PhospherusAllocator, hashChainNode);
        ++ret->openCnt;
        return ret;
    }

    __DeviceInfo* deviceInfo = NULL;
    __SuperNodeInfo* firstFreeSuperNode = NULL;
    __BlockStack * firstFreeBlockStack = NULL;

    bool abort = false;
    do {
        if (abort = (
            (ret = kMalloc(sizeof(PhospherusAllocator), MEMORY_TYPE_NORMAL)) == NULL
            )) {
            break;
        }

        if (abort = (
            (deviceInfo = kMalloc(sizeof(__DeviceInfo), MEMORY_TYPE_NORMAL)) == NULL
            )) {
            break;
        }

        if (abort = (
            (firstFreeSuperNode = kMalloc(sizeof(__SuperNode), MEMORY_TYPE_NORMAL)) == NULL
            )) {
            break;
        }

        if (abort = (
            (firstFreeBlockStack = kMalloc(sizeof(__BlockStack), MEMORY_TYPE_NORMAL)) == NULL
            )) {
            break;
        }

        if (abort = (
            blockDeviceReadBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE) == RESULT_FAIL
            )) {
            break;
        }

        if (deviceInfo->firstFreeSuperNode != INVALID_INDEX) {
            if (abort = (
                blockDeviceReadBlocks(device, deviceInfo->firstFreeSuperNode, firstFreeSuperNode, __SUPER_NODE_SIZE) == RESULT_FAIL
                )) {
                break;
            }
        }

        if (deviceInfo->firstFreeBlockStack != INVALID_INDEX) {
            if (abort = (
                blockDeviceReadBlocks(device, deviceInfo->firstFreeBlockStack, firstFreeBlockStack, __BLOCK_STACK_SIZE) == RESULT_FAIL
                )) {
                break;
            }
        }

        ret->device = device;
        ret->deviceInfo = deviceInfo;
        ret->firstFreeSuperNode = firstFreeSuperNode;
        ret->firstFreeBlockStack = firstFreeBlockStack;
        ret->openCnt = 1;
        initHashChainNode(&ret->hashChainNode);

        if (abort = 
            !hashTableInsert(&_hashTable, (Object)device->deviceID, &ret->hashChainNode)
            ) {

            SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
            break;
        }
    } while (0);

    if (abort) {
        if (firstFreeBlockStack != NULL) {
            kFree(firstFreeBlockStack);
        }

        if (firstFreeSuperNode != NULL) {
            kFree(firstFreeSuperNode);
        }

        if (deviceInfo != NULL) {
            kFree(deviceInfo);
        }

        if (ret != NULL) {
            kFree(ret);
        }

        return NULL;
    }

    return ret;
}

Result phospherusCloseAllocator(PhospherusAllocator* allocator) {
    if (--allocator->openCnt > 0) {
        return RESULT_SUCCESS;
    }

    BlockDevice* device = allocator->device;
    if (hashTableDelete(&_hashTable, (Object)device->deviceID) == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DATA, ERROR_STATUS_NOT_FOUND);
        return RESULT_FAIL;
    }

    kFree(allocator->deviceInfo);
    kFree(allocator->firstFreeSuperNode);
    kFree(allocator->firstFreeBlockStack);
    allocator->deviceInfo = allocator->firstFreeSuperNode = allocator->firstFreeBlockStack = NULL;
    kFree(allocator);

    return RESULT_SUCCESS;
}

Index64 phospherusAllocateCluster(PhospherusAllocator* allocator) {
    BlockDevice* device = allocator->device;
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    __SuperNode* firstFreeSuperNode = allocator->firstFreeSuperNode;

    if (deviceInfo->deviceFreeClusterNum == 0 || deviceInfo->firstFreeSuperNode == INVALID_INDEX) {         //Theoretically, these two can be true iff at the same time
        return INVALID_INDEX;
    }

    __ClusterStack* stack = &firstFreeSuperNode->stack;
    __SuperNodeInfo* info = &firstFreeSuperNode->info;

    Index64 ret = POP_CLUSTER(stack);   //Pop one cluster from stack
    if (blockDeviceWriteBlocks(device, info->blockIndex + __SUPER_NODE_STACK_OFFSET, stack, __SUPER_NODE_STACK_SIZE) == RESULT_FAIL) {  //Save the stack
        return INVALID_INDEX;
    }

    bool empty = CLUSTER_STACK_EMPTY(stack);
    if (empty) {
        deviceInfo->firstFreeSuperNode = info->nextFreeSuperNode;
        if (deviceInfo->firstFreeSuperNode == INVALID_INDEX) {
            deviceInfo->lastFreeSuperNode = INVALID_INDEX;
            memset(firstFreeSuperNode, 0, sizeof(__SuperNode));     //Destroy the invalid data
        }
    }

    --deviceInfo->deviceFreeClusterNum;
    //deviceInfo is very important for file system to work, if it is modified, no other errors shou happens before it is wrote back
    if (blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE) == RESULT_FAIL) {    //Save root super node
        return INVALID_INDEX;
    }

    if (empty) {
        //Read new free super block, no need to modify old free super block's info
        if (deviceInfo->firstFreeSuperNode != INVALID_INDEX && blockDeviceReadBlocks(device, deviceInfo->firstFreeSuperNode, firstFreeSuperNode, __SUPER_NODE_SIZE) == RESULT_FAIL) {
            return INVALID_INDEX;
        }
    }

    return ret;
}

Result phospherusReleaseCluster(PhospherusAllocator* allocator, Index64 cluster) {
    void* belongTo = NULL, * tailSuperNodeInfo = NULL;

    Result ret = __doReleaseCluster(allocator, cluster, &belongTo, &tailSuperNodeInfo);

    if (belongTo != NULL) {
        releaseBuffer(belongTo, BUFFER_SIZE_4096);
    }

    if (tailSuperNodeInfo != NULL) {
        releaseBuffer(tailSuperNodeInfo, BUFFER_SIZE_512);
    }
}

Size phospherusGetFreeClusterNum(PhospherusAllocator* allocator) {
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    return deviceInfo->deviceFreeClusterNum;
}

Index64 phospherusAllocateBlock(PhospherusAllocator* allocator) {
    void* blockStack = NULL;

    Index64 ret = __doAllocateBlock(allocator, &blockStack);

    if (blockStack != NULL) {
        releaseBuffer(blockStack, BUFFER_SIZE_512);
    }

    return ret;
}

Result phospherusReleaseBlock(PhospherusAllocator* allocator, Index64 block) {
    void* belongTo = NULL, * tailBlockStack = NULL;

    Result ret = __doReleaseBlock(allocator, block, &belongTo, &tailBlockStack);

    if (belongTo != NULL) {
        releaseBuffer(belongTo, BUFFER_SIZE_512);
    }

    if (tailBlockStack != NULL) {
        releaseBuffer(tailBlockStack, BUFFER_SIZE_512);
    }

    return ret;
}

static Result __doDeployAllocator(BlockDevice* device, __DeviceInfo** infoPtr, __SuperNode** superNodePtr) {
    Size deviceSize = device->availableBlockNum;
    Size nodeNum = deviceSize / __SUPER_NODE_SPAN;

    if (nodeNum < 1) {   //The block device must have at least one node length free (1MB)
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NO_FREE_SPACE);
        return RESULT_FAIL;
    }

    __DeviceInfo* info = NULL;
    *infoPtr = info = allocateBuffer(BUFFER_SIZE_512);
    if (info == NULL) {
        return RESULT_FAIL;
    }

    info->signature = SYSTEM_INFO_MAGIC64;
    //First free cluster of root super node is reserved for special purpose like root directory
    info->deviceFreeClusterNum = deviceSize / CLUSTER_BLOCK_SIZE - nodeNum - 1;
    info->firstFreeSuperNode = 0, info->lastFreeSuperNode = (nodeNum - 1) * __SUPER_NODE_SPAN;
    info->firstFreeBlockStack = info->lastFreeBlockStack = INVALID_INDEX;

    if (blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, info, __DEVICE_INFO_SIZE) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    __SuperNode* superNode = NULL;
    *superNodePtr = superNode = allocateBuffer(BUFFER_SIZE_4096);
    if (superNode == NULL) {
        return RESULT_FAIL;
    }
    memset(superNode, 0, sizeof(__SuperNode));

    //Public info
    __SuperNodeInfo* nodeInfo = &superNode->info;
    __ClusterStack* stack = &superNode->stack;
    nodeInfo->signature1 = nodeInfo->signature2 = SYSTEM_INFO_MAGIC64;

    for (int i = 0; i < nodeNum; ++i) {
        nodeInfo->blockIndex = i * __SUPER_NODE_SPAN;
        nodeInfo->nextFreeSuperNode = i + 1 == nodeNum ? INVALID_INDEX : (i + 1) * __SUPER_NODE_SPAN;

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

        if (blockDeviceWriteBlocks(device, nodeInfo->blockIndex, superNode, __SUPER_NODE_SIZE) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return RESULT_SUCCESS;
}

static Result __doReleaseCluster(PhospherusAllocator* allocator, Index64 cluster, void** belongToPtr, void** tailSuperNodeInfoPtr) {
    if (cluster == INVALID_INDEX) {     //Well, just in case
        SET_ERROR_CODE(ERROR_OBJECT_ARGUMENT, ERROR_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    BlockDevice* device = allocator->device;
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    __SuperNode* firstFreeSuperNode = allocator->firstFreeSuperNode;

    __SuperNode* belongTo = NULL;
    Index64 belongToIndex = (cluster / __SUPER_NODE_SPAN) * __SUPER_NODE_SPAN;  //Which super node does this cluster belongs to
    if (belongToIndex == firstFreeSuperNode->info.blockIndex) {
        belongTo = firstFreeSuperNode;
    } else {
        *belongToPtr = belongTo = allocateBuffer(BUFFER_SIZE_4096);
        if (belongTo == NULL || blockDeviceReadBlocks(device, belongToIndex, belongTo, __SUPER_NODE_SIZE) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    bool empty = CLUSTER_STACK_EMPTY(&belongTo->stack);
    PUSH_CLUSTER(&belongTo->stack, cluster);
    if (empty) {
        belongTo->info.nextFreeSuperNode = INVALID_INDEX;
    }

    if (blockDeviceWriteBlocks(device, belongToIndex, belongTo, __SUPER_NODE_SIZE) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (empty) {
        //The release make a super block free, need to be inserted back to the list
        if (deviceInfo->firstFreeSuperNode == INVALID_INDEX) {  //When deivce has no free super node
            deviceInfo->firstFreeSuperNode = deviceInfo->lastFreeSuperNode = belongToIndex;
        } else {
            __SuperNodeInfo* tailSuperNodeInfo = NULL;
            if (deviceInfo->firstFreeSuperNode == deviceInfo->lastFreeSuperNode) {
                tailSuperNodeInfo = &firstFreeSuperNode->info;
            } else {
                *tailSuperNodeInfoPtr = tailSuperNodeInfo = allocateBuffer(BUFFER_SIZE_512);
                if (tailSuperNodeInfo == NULL || blockDeviceReadBlocks(device, deviceInfo->lastFreeSuperNode + __SUPER_NODE_INFO_OFFSET, tailSuperNodeInfo, __SUPER_NODE_INFO_SIZE) == RESULT_FAIL) {
                    return RESULT_FAIL;
                }
            }

            tailSuperNodeInfo->nextFreeSuperNode = belongToIndex;

            if (blockDeviceWriteBlocks(device, deviceInfo->lastFreeSuperNode + __SUPER_NODE_INFO_OFFSET, tailSuperNodeInfo, __SUPER_NODE_INFO_SIZE) == RESULT_FAIL) {
                return RESULT_FAIL;
            }

            deviceInfo->lastFreeSuperNode = belongToIndex;
        }
    }

    ++deviceInfo->deviceFreeClusterNum;
    if (blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE) == RESULT_FAIL) {    //Save root super node
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Index64 __doAllocateBlock(PhospherusAllocator* allocator, void** blockStackPtr) {
    BlockDevice* device = allocator->device;
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    __BlockStack* firstFreeBlockStack = allocator->firstFreeBlockStack;

    if (deviceInfo->firstFreeBlockStack == INVALID_INDEX) { //No block stacks
        Index64 cluster = phospherusAllocateCluster(allocator);
        if (cluster == INVALID_INDEX) {
            return INVALID_INDEX;
        }

        __BlockStack* blockStack = NULL;
        *blockStackPtr = blockStack = (__BlockStack*)allocateBuffer(BUFFER_SIZE_512);
        if (blockStack == NULL) {
            return INVALID_INDEX;
        }
        memset(blockStack, 0, sizeof(__BlockStack));

        blockStack->nextFreeBlockStack = INVALID_INDEX;
        blockStack->blockIndex = cluster;
        blockStack->stackTop = 6;
        for (int i = 0; i < CLUSTER_BLOCK_SIZE - 1; ++i) {
            blockStack->blockStack[i] = cluster + i + 1;
        }

        if (blockDeviceWriteBlocks(device, cluster, blockStack, __BLOCK_STACK_SIZE) == RESULT_FAIL) {
            return INVALID_INDEX;
        }

        memcpy(firstFreeBlockStack, blockStack, sizeof(__BlockStack));
        deviceInfo->firstFreeBlockStack = deviceInfo->lastFreeBlockStack = blockStack->blockIndex;

        if (blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE) == RESULT_FAIL) {
            return INVALID_INDEX;
        }
    }

    Index64 ret = POP_BLOCK(firstFreeBlockStack);
    if (blockDeviceWriteBlocks(device, firstFreeBlockStack->blockIndex, firstFreeBlockStack, __BLOCK_STACK_SIZE) == RESULT_FAIL) {
        return INVALID_INDEX;
    }

    if (BLOCK_STACK_EMPTY(firstFreeBlockStack)) {   //If the pop make stack empty
        deviceInfo->firstFreeBlockStack = firstFreeBlockStack->nextFreeBlockStack;
        if (deviceInfo->firstFreeBlockStack == INVALID_INDEX) {
            deviceInfo->lastFreeBlockStack = INVALID_INDEX;     //Destroy invalid data
            memset(firstFreeBlockStack, 0, sizeof(__BlockStack));
        }

        //deviceInfo is very important for file system to work, if it is modified, no other errors shou happens before it is wrote back
        if (blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE) == RESULT_FAIL) {
            return INVALID_INDEX;
        }

        if (deviceInfo->firstFreeBlockStack != INVALID_INDEX && blockDeviceReadBlocks(device, firstFreeBlockStack->nextFreeBlockStack, firstFreeBlockStack, __BLOCK_STACK_SIZE) == RESULT_FAIL) {
            return INVALID_INDEX;
        }
    }

    return ret;
}

static Result __doReleaseBlock(PhospherusAllocator* allocator, Index64 block, void** belongToPtr, void** tailBlockStackPtr) {
    if (block == INVALID_INDEX) {     //Well, just in case
        SET_ERROR_CODE(ERROR_OBJECT_ARGUMENT, ERROR_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    BlockDevice* device = allocator->device;
    __DeviceInfo* deviceInfo = allocator->deviceInfo;
    __BlockStack* firstFreeBlockStack = allocator->firstFreeBlockStack;

    __BlockStack* belongTo = NULL;
    Index64 belongToIndex = (block / CLUSTER_BLOCK_SIZE) * CLUSTER_BLOCK_SIZE;
    if (belongToIndex == deviceInfo->firstFreeBlockStack) {
        belongTo = firstFreeBlockStack;
    } else {
        *belongToPtr = belongTo = allocateBuffer(BUFFER_SIZE_512);
        if (belongTo == NULL || blockDeviceReadBlocks(device, belongToIndex, belongTo, __BLOCK_STACK_SIZE) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    bool empty = BLOCK_STACK_EMPTY(belongTo);
    PUSH_BLOCK(belongTo, block);
    if (empty) {
        belongTo->nextFreeBlockStack = INVALID_INDEX;
    }

    if (blockDeviceWriteBlocks(device, belongToIndex, belongTo, __BLOCK_STACK_SIZE) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (empty) {
        if (deviceInfo->lastFreeBlockStack == INVALID_INDEX) {
            deviceInfo->firstFreeBlockStack = deviceInfo->lastFreeBlockStack = belongToIndex;
        } else {
            __BlockStack* tailBlockStack = NULL;
            if (deviceInfo->firstFreeBlockStack == deviceInfo->lastFreeBlockStack) {
                tailBlockStack = firstFreeBlockStack;
            } else {
                *tailBlockStackPtr = tailBlockStack = allocateBuffer(BUFFER_SIZE_512);
                if (tailBlockStack == NULL || blockDeviceReadBlocks(device, deviceInfo->lastFreeBlockStack, tailBlockStack, __BLOCK_STACK_SIZE) == RESULT_FAIL) {
                    return RESULT_FAIL;
                }
            }

            tailBlockStack->nextFreeBlockStack = belongToIndex;

            if (blockDeviceWriteBlocks(device, deviceInfo->lastFreeBlockStack, tailBlockStack, __BLOCK_STACK_SIZE) == RESULT_FAIL) {
                return RESULT_FAIL;
            }

            deviceInfo->lastFreeBlockStack = belongToIndex;
        }

        if (blockDeviceWriteBlocks(device, RESERVED_BLOCK_DEVICE_INFO, deviceInfo, __DEVICE_INFO_SIZE) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return RESULT_SUCCESS;
}