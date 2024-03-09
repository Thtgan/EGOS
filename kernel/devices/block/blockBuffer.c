#include<devices/block/blockBuffer.h>

#include<algorithms.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<structs/linkedList.h>
#include<structs/hashTable.h>
#include<system/pageTable.h>

static void __initBlock(Block* block, void* data, Index64 blockIndex);

Result initBlockBuffer(BlockBuffer* blockBuffer, Size chainNum, Size blockNum, Size bytePerBlockShift) {
    blockBuffer->bytePerBlockShift  = bytePerBlockShift;
    void* pBlockData = physicalPage_alloc(DIVIDE_ROUND_UP_SHIFT((blockNum << bytePerBlockShift), PAGE_SIZE_SHIFT), PHYSICAL_PAGE_ATTRIBUTE_PUBLIC);
    if (pBlockData == NULL) {
        return RESULT_FAIL;
    }

    blockBuffer->blockData          = convertAddressP2V(pBlockData);
    linkedList_initStruct(&blockBuffer->LRU);

    Block* blocks = kMalloc(sizeof(Block) * blockNum);
    if (blocks == NULL) {
        return RESULT_FAIL;
    }

    for (int i = 0; i < blockNum; ++i) {
        Block* block = blocks + i;
        __initBlock(block, blockBuffer->blockData + (i << bytePerBlockShift), INVALID_INDEX);

        linkedListNode_insertBack(&blockBuffer->LRU, &block->LRUnode);
    }

    SinglyLinkedList* chains = kMalloc(sizeof(SinglyLinkedList) * chainNum);
    if (chains == NULL) {
        return RESULT_FAIL;
    }

    hashTable_initStruct(&blockBuffer->hashTable, chainNum, chains, LAMBDA(Size, (HashTable* this, Object key) {
        return key % this->hashSize;
    }));
    blockBuffer->blockNum           = blockNum;

    return RESULT_SUCCESS;
}

void releaseBlockBuffer(BlockBuffer* blockBuffer) {
    Uintptr minOldBlockPtr = -1;
    LinkedListNode* current = linkedListNode_getNext(&blockBuffer->LRU);
    for (int i = 0; i < blockBuffer->blockNum; ++i) {
        minOldBlockPtr = umin64(minOldBlockPtr, (Uintptr)HOST_POINTER(current, Block, LRUnode));
        current = linkedListNode_getNext(current);
    }

    kFree((void*)minOldBlockPtr);
    kFree(blockBuffer->hashTable.chains);
    physicalPage_free(convertAddressV2P(blockBuffer->blockData));

    memset(blockBuffer, 0, sizeof(BlockBuffer));
}

Result resizeBlockBuffer(BlockBuffer* blockBuffer, Size newBlockNum) {
    Block* newBlocks = kMalloc(sizeof(Block) * newBlockNum);
    if (newBlocks == NULL) {
        return RESULT_FAIL;
    }

    LinkedListNode* current = linkedListNode_getNext(&blockBuffer->LRU);
    Uintptr minOldBlockPtr = -1;
    Size maxBlockNum = umax64(blockBuffer->blockNum, newBlockNum);
    for (int i = 0; i < maxBlockNum; ++i) {
        Block* oldBlock = NULL, * newBlock = NULL;
        if (i < blockBuffer->blockNum) {
            oldBlock = HOST_POINTER(linkedListNode_getNext(&blockBuffer->LRU), Block, LRUnode);
        }

        if (i < newBlockNum) {
            newBlock = newBlocks + i;
        }

        if (newBlock != NULL) {
            if (oldBlock != NULL) {
                __initBlock(newBlock, oldBlock->data, oldBlock->blockIndex);
            } else {
                __initBlock(newBlock, NULL, INVALID_INDEX);
            }

            linkedListNode_insertFront(&blockBuffer->LRU, &newBlock->LRUnode);
        }

        if (oldBlock != NULL) {
            minOldBlockPtr = umin64(minOldBlockPtr, (Uintptr)oldBlock);
            linkedListNode_delete(&oldBlock->LRUnode);
        }
    }

    kFree((void*)minOldBlockPtr);
    blockBuffer->blockNum = newBlockNum;

    return RESULT_SUCCESS;
}

Block* blockBufferSearch(BlockBuffer* blockBuffer, Index64 blockIndex) {
    HashChainNode* found = hashTable_find(&blockBuffer->hashTable, blockIndex);
    if (found == NULL) {
        return NULL;
    }

    Block* block = HOST_POINTER(found, Block, hashChainNode);
    linkedListNode_delete(&block->LRUnode);
    linkedListNode_initStruct(&block->LRUnode);
    linkedListNode_insertBack(&blockBuffer->LRU, &block->LRUnode);

    return block;
}

Block* blockBufferPop(BlockBuffer* blockBuffer, Index64 blockIndex) {   //If blockIndex is INVALID_INDEX, it pops the tail of the LRU list
    HashChainNode* found = hashTable_find(&blockBuffer->hashTable, blockIndex);

    Block* block;
    if (found == NULL) {
        block = HOST_POINTER(linkedListNode_getPrev(&blockBuffer->LRU), Block, LRUnode);
    } else {
        block = HOST_POINTER(found, Block, hashChainNode);
    }

    if (block->blockIndex != INVALID_INDEX && hashTable_delete(&blockBuffer->hashTable, block->blockIndex) != &block->hashChainNode) {
        return NULL;
    }
    linkedListNode_delete(&block->LRUnode);

    hashChainNode_initStruct(&block->hashChainNode);
    linkedListNode_initStruct(&block->LRUnode);

    if (block->blockIndex != blockIndex) {
        CLEAR_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT);
    }

    return block;
}

Result blockBufferPush(BlockBuffer* blockBuffer, Index64 blockIndex, Block* block) {
    block->blockIndex = blockIndex;
    if (blockIndex != INVALID_INDEX && hashTable_insert(&blockBuffer->hashTable, block->blockIndex, &block->hashChainNode) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    linkedListNode_insertBack(&blockBuffer->LRU, &block->LRUnode);
    if (blockIndex != INVALID_INDEX) {
        SET_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT);
    }

    return RESULT_SUCCESS;
}

static void __initBlock(Block* block, void* data, Index64 blockIndex) {
    block->data         = data;
    block->blockIndex   = blockIndex;
    linkedListNode_initStruct(&block->LRUnode);
    hashChainNode_initStruct(&block->hashChainNode);
    block->flags        = EMPTY_FLAGS;
}