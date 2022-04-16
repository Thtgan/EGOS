#include<fs/phospherus/allocator.h>

#include<blowup.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/blockLinkedList.h>
#include<fs/phospherus/phospherus.h>
#include<kit/bit.h>
#include<memory/buffer.h>
#include<memory/malloc.h>
#include<memory/memory.h>
#include<stddef.h>
#include<stdint.h>
#include<system/systemInfo.h>

#define __SUPER_NODE_INDEX  0

typedef struct {
    uint32_t signature;
    block_index_t blockIndex;
    size_t freeBlockNum;
    Phospherus_BlockLinkedListNode nodeListNode;
    uint8_t reserved[220];
    size_t freeBlockNumInNode;
    uint8_t bitmap[256];
} __attribute__((packed)) __Node;

#define __BLOCK_LIST_REMOVE_NEXT(__DEVICE, __NODE_PTR) phospherus_blockLinkedListNodeRemoveNext(__DEVICE, &(__NODE_PTR)->nodeListNode, offsetof(__Node, nodeListNode))
#define __BLOCK_LIST_INSERT_NEXT(__DEVICE, __NODE_PTR, __NEW_NODE_INDEX) phospherus_blockLinkedListNodeInsertNext(__DEVICE, &(__NODE_PTR)->nodeListNode, __NEW_NODE_INDEX, offsetof(__Node, nodeListNode))

/**
 * @brief Initialize a node
 * 
 * @param node Empty node
 * @param nodeIndex The index of block storing the node
 */
void __initNode(__Node* node, block_index_t nodeIndex);

/**
 * @brief Allocate a block from the node
 * 
 * @param node Node
 * @return block_index_t Block allocated from the node
 */
block_index_t __allocateBlockFromNode(__Node* node);

/**
 * @brief Release a block allocated from the node
 * 
 * @param node Node
 * @param blockIndex Allocated block
 */
void __releaseBlockToNode(__Node* node, block_index_t blockIndex);

static const uint8_t _emptyBlock[512];

void phospherus_initAllocator() {
    uint64_t ptr = (uint64_t)_emptyBlock;
    memset((void*)ptr, 0, sizeof(_emptyBlock));
}

bool phospherus_checkBlockDevice(BlockDevice* device) {
    __Node* superNode = allocateBuffer(BUFFER_SIZE_512);
    device->readBlocks(device->additionalData, __SUPER_NODE_INDEX, superNode, 1);

    bool ret = (superNode->signature == SYSTEM_INFO_MAGIC32);   //Check by reading the signature

    releaseBuffer(superNode, BUFFER_SIZE_512);

    return ret;
}

//TODO: These read/write are ugly, simplify them
bool phospherus_deployAllocator(BlockDevice* device) {
    size_t deviceSize = device->availableBlockNum;
    size_t nodeNum = deviceSize / ALLOCATOR_NODE_SIZE_IN_BLOCK;

    if (nodeNum < 1) {   //The block device must have at least one node length free
        return false;
    }

    __Node* node = allocateBuffer(BUFFER_SIZE_512);
    __initNode(node, __SUPER_NODE_INDEX);

    size_t freeBlockNum = nodeNum * (ALLOCATOR_NODE_SIZE_IN_BLOCK - 1);
    for (int i = 0; i < nodeNum; ++i) {
        //Number make sense only for first node (supernode)
        node->freeBlockNum = (i == __SUPER_NODE_INDEX) ? freeBlockNum : PHOSPHERUS_NULL;

        //Record position
        node->blockIndex = i * ALLOCATOR_NODE_SIZE_IN_BLOCK;

        //Setting up the linked list
        phospherus_blockLinkedListNodeSetNext(&node->nodeListNode, i + 1 < nodeNum ? (i + 1) * ALLOCATOR_NODE_SIZE_IN_BLOCK : PHOSPHERUS_NULL);

        device->writeBlocks(device->additionalData, i * ALLOCATOR_NODE_SIZE_IN_BLOCK, node, 1);  //Deploy a node each 2048 blocks
    }

    releaseBuffer(node, BUFFER_SIZE_512);

    return true;
}

Phospherus_Allocator* phospherus_openAllocator(BlockDevice* device) {
    __Node* superNode = malloc(sizeof(__Node));
    Phospherus_Allocator* ret = malloc(sizeof(Phospherus_Allocator));

    device->readBlocks(device->additionalData, __SUPER_NODE_INDEX, superNode, 1);

    ret->device = device;
    ret->superNode = superNode;

    return ret;
}

void phospherus_closeAllocator(Phospherus_Allocator* allocator) {
    if (allocator == NULL || allocator->superNode == NULL) {
        return;
    }
    free(allocator->superNode); //Destroy supernode will make allocator unavailable
    allocator->superNode = NULL;
    free(allocator);
}

static const char* _allocateBlockFailInfo = "Allocate block failed";

block_index_t phospherus_allocateBlock(Phospherus_Allocator* allocator) {
    BlockDevice* device = allocator->device;
    __Node* superNode = (__Node*)allocator->superNode;
    if (superNode->freeBlockNum == 0) {
        return PHOSPHERUS_NULL;
    }

    block_index_t ret = PHOSPHERUS_NULL;
    if (superNode->freeBlockNumInNode > 0) {    //Getting the block from super node
        ret = __allocateBlockFromNode(superNode);
    } else {                                    //Getting the block from other nodes
        block_index_t blockNode = phospherus_blockLinkedListNodeGetNext(&superNode->nodeListNode);
        if (blockNode == PHOSPHERUS_NULL) {
            blowup(_allocateBlockFailInfo);
        }
        __Node* node = allocateBuffer(BUFFER_SIZE_512);
        device->readBlocks(device->additionalData, blockNode, node, 1);
        ret = __allocateBlockFromNode(node);

        size_t freeBlockNumInNode = node->freeBlockNumInNode;
        device->writeBlocks(device->additionalData, blockNode, node, 1);

        if (freeBlockNumInNode == 0) {   //If the node cannot allocate any more blocks, remove it
            __BLOCK_LIST_REMOVE_NEXT(device, superNode);
        }

        releaseBuffer(node, BUFFER_SIZE_512);
    }

    if (ret == PHOSPHERUS_NULL) {
        blowup(_allocateBlockFailInfo);
    }

    --superNode->freeBlockNum;
    device->writeBlocks(device->additionalData, __SUPER_NODE_INDEX, superNode, 1);

    device->writeBlocks(device->additionalData, ret, _emptyBlock, 1);   //Clear the block

    return ret;
}

//TODO: Add the handle to multiple release to same block
void phospherus_releaseBlock(Phospherus_Allocator* allocator, block_index_t blockIndex) {
    if (blockIndex == PHOSPHERUS_NULL) {
        return;
    }
    
    BlockDevice* device = allocator->device;
    __Node* superNode = (__Node*)allocator->superNode;

    block_index_t nodeIndex = (blockIndex / ALLOCATOR_NODE_SIZE_IN_BLOCK) * ALLOCATOR_NODE_SIZE_IN_BLOCK;  //Which node is this block belong

    if (nodeIndex == __SUPER_NODE_INDEX) {    //If the block comes from super node, just release it
        __releaseBlockToNode(superNode, blockIndex);
    } else {
        __Node* node = allocateBuffer(BUFFER_SIZE_512);
        device->readBlocks(device->additionalData, nodeIndex, node, 1);

        __releaseBlockToNode(node, blockIndex);
        device->writeBlocks(device->additionalData, nodeIndex, node, 1);

        if (node->freeBlockNumInNode == 1) {  //The empty node has the first released block, insert back to the node list
            __BLOCK_LIST_INSERT_NEXT(device, superNode, nodeIndex);
        }

        releaseBuffer(node, BUFFER_SIZE_512);
    }

    ++superNode->freeBlockNum;
    
    device->writeBlocks(device->additionalData, __SUPER_NODE_INDEX, superNode, 1);
}

void __initNode(__Node* node, block_index_t nodeIndex) {
    memset(node, 0, sizeof(__Node));
    node->signature = SYSTEM_INFO_MAGIC32;
    node->blockIndex = nodeIndex;

    node->freeBlockNum = PHOSPHERUS_NULL;
    phospherus_initBlockLinkedListNode(&node->nodeListNode);

    node->freeBlockNumInNode = ALLOCATOR_NODE_SIZE_IN_BLOCK - 1;
}

block_index_t __allocateBlockFromNode(__Node* node) {
    uint64_t* ptr1 = (uint64_t*)node->bitmap;
    block_index_t index1 = 0;
    for (; index1 < 32; ++index1) {
        if (ptr1[index1] != FULL_MASK(64)) {
            break;
        }
    }

    uint8_t* ptr2 = (uint8_t*)&ptr1[index1];
    block_index_t index2 = 0;
    for (; index2 < 8; ++index2) {
        if (ptr2[index2] != FULL_MASK(8)) {
            break;
        }
    }

    uint8_t subMap = ptr2[index2];
    block_index_t index3 = 0;
    for (; index3 < 8; ++index3) {
        if (TEST_FLAGS_FAIL(subMap, FLAG8(index3))) {
            break;
        }
    }

    block_index_t bitIndex = index1 * 64 + index2 * 8 + index3;
    if (bitIndex < ALLOCATOR_NODE_SIZE_IN_BLOCK - 1) {
        SET_FLAG_BACK(node->bitmap[bitIndex / 8], FLAG8(bitIndex % 8));
        --node->freeBlockNumInNode;
        return node->blockIndex + bitIndex + 1;
    }

    return PHOSPHERUS_NULL;
}

void __releaseBlockToNode(__Node* node, block_index_t blockIndex) {
    block_index_t bitIndex = blockIndex - node->blockIndex - 1;
    CLEAR_FLAG_BACK(node->bitmap[bitIndex / 8], FLAG8(bitIndex % 8));
    ++node->freeBlockNumInNode;
}

size_t phospherus_getFreeBlockNum(Phospherus_Allocator* allocator) {
    __Node* superNode = (__Node*)allocator->superNode;
    return superNode->freeBlockNum;
}

BlockDevice* phospherus_getAllocatorDevice(Phospherus_Allocator* allocator) {
    return allocator->device;
}