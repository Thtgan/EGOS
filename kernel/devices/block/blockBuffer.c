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
    void* pBlockData = pageAlloc(DIVIDE_ROUND_UP_SHIFT((blockNum << bytePerBlockShift), PAGE_SIZE_SHIFT), MEMORY_TYPE_PUBLIC);
    if (pBlockData == NULL) {
        return RESULT_FAIL;
    }

    blockBuffer->blockData          = convertAddressP2V(pBlockData);
    initLinkedList(&blockBuffer->LRU);

    Block* blocks = kMalloc(sizeof(Block) * blockNum);
    if (blocks == NULL) {
        return RESULT_FAIL;
    }

    for (int i = 0; i < blockNum; ++i) {
        Block* block = blocks + i;
        __initBlock(block, blockBuffer->blockData + (i << bytePerBlockShift), INVALID_INDEX);

        linkedListInsertBack(&blockBuffer->LRU, &block->LRUnode);
    }

    SinglyLinkedList* chains = kMalloc(sizeof(SinglyLinkedList) * chainNum);
    if (chains == NULL) {
        return RESULT_FAIL;
    }

    initHashTable(&blockBuffer->hashTable, chainNum, chains, LAMBDA(Size, (HashTable* this, Object key) {
        return key % this->hashSize;
    }));
    blockBuffer->blockNum           = blockNum;

    return RESULT_SUCCESS;
}

void releaseBlockBuffer(BlockBuffer* blockBuffer) {
    Uintptr minOldBlockPtr = -1;
    LinkedListNode* current = linkedListGetNext(&blockBuffer->LRU);
    for (int i = 0; i < blockBuffer->blockNum; ++i) {
        minOldBlockPtr = umin64(minOldBlockPtr, (Uintptr)HOST_POINTER(current, Block, LRUnode));
        current = linkedListGetNext(current);
    }

    kFree((void*)minOldBlockPtr);
    kFree(blockBuffer->hashTable.chains);
    pageFree(convertAddressV2P(blockBuffer->blockData));

    memset(blockBuffer, 0, sizeof(BlockBuffer));
}

Result resizeBlockBuffer(BlockBuffer* blockBuffer, Size newBlockNum) {
    Block* newBlocks = kMalloc(sizeof(Block) * newBlockNum);
    if (newBlocks == NULL) {
        return RESULT_FAIL;
    }

    LinkedListNode* current = linkedListGetNext(&blockBuffer->LRU);
    Uintptr minOldBlockPtr = -1;
    Size maxBlockNum = umax64(blockBuffer->blockNum, newBlockNum);
    for (int i = 0; i < maxBlockNum; ++i) {
        Block* oldBlock = NULL, * newBlock = NULL;
        if (i < blockBuffer->blockNum) {
            oldBlock = HOST_POINTER(linkedListGetNext(&blockBuffer->LRU), Block, LRUnode);
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

            linkedListInsertFront(&blockBuffer->LRU, &newBlock->LRUnode);
        }

        if (oldBlock != NULL) {
            minOldBlockPtr = umin64(minOldBlockPtr, (Uintptr)oldBlock);
            linkedListDelete(&oldBlock->LRUnode);
        }
    }

    kFree((void*)minOldBlockPtr);
    blockBuffer->blockNum = newBlockNum;

    return RESULT_SUCCESS;
}

Block* blockBufferSearch(BlockBuffer* blockBuffer, Index64 blockIndex) {
    HashChainNode* found = hashTableFind(&blockBuffer->hashTable, blockIndex);
    if (found == NULL) {
        return NULL;
    }

    Block* block = HOST_POINTER(found, Block, hashChainNode);
    linkedListDelete(&block->LRUnode);
    initLinkedListNode(&block->LRUnode);
    linkedListInsertBack(&blockBuffer->LRU, &block->LRUnode);

    return block;
}

Block* blockBufferPop(BlockBuffer* blockBuffer, Index64 blockIndex) {   //If blockIndex is INVALID_INDEX, it pops the tail of the LRU list
    HashChainNode* found = hashTableFind(&blockBuffer->hashTable, blockIndex);

    Block* block;
    if (found == NULL) {
        block = HOST_POINTER(linkedListGetPrev(&blockBuffer->LRU), Block, LRUnode);
    } else {
        block = HOST_POINTER(found, Block, hashChainNode);
    }

    if (block->blockIndex != INVALID_INDEX && hashTableDelete(&blockBuffer->hashTable, block->blockIndex) != &block->hashChainNode) {
        return NULL;
    }
    linkedListDelete(&block->LRUnode);

    initHashChainNode(&block->hashChainNode);
    initLinkedListNode(&block->LRUnode);

    if (block->blockIndex != blockIndex) {
        CLEAR_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT);
    }

    return block;
}

Result blockBufferPush(BlockBuffer* blockBuffer, Index64 blockIndex, Block* block) {
    block->blockIndex = blockIndex;
    if (blockIndex != INVALID_INDEX && hashTableInsert(&blockBuffer->hashTable, block->blockIndex, &block->hashChainNode) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    linkedListInsertBack(&blockBuffer->LRU, &block->LRUnode);
    if (blockIndex != INVALID_INDEX) {
        SET_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT);
    }

    return RESULT_SUCCESS;
}

static void __initBlock(Block* block, void* data, Index64 blockIndex) {
    block->data         = data;
    block->blockIndex   = blockIndex;
    initLinkedListNode(&block->LRUnode);
    initHashChainNode(&block->hashChainNode);
    block->flags        = EMPTY_FLAGS;
}