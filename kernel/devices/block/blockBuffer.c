#include<devices/block/blockBuffer.h>

#include<algorithms.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<structs/linkedList.h>
#include<structs/hashTable.h>
#include<system/pageTable.h>

static void __blockBuffer_initBlock(Block* block, void* data, Index64 blockIndex);

Result blockBuffer_initStruct(BlockBuffer* blockBuffer, Size chainNum, Size blockNum, Size bytePerBlockShift) {
    blockBuffer->bytePerBlockShift  = bytePerBlockShift;
    void* pBlockData = memory_allocateFrame(DIVIDE_ROUND_UP_SHIFT((blockNum << bytePerBlockShift), PAGE_SIZE_SHIFT));
    if (pBlockData == NULL) {
        return RESULT_FAIL;
    }

    blockBuffer->blockData          = paging_convertAddressP2V(pBlockData);
    linkedList_initStruct(&blockBuffer->LRU);

    Block* blocks = memory_allocate(sizeof(Block) * blockNum);
    if (blocks == NULL) {
        return RESULT_FAIL;
    }

    for (int i = 0; i < blockNum; ++i) {
        Block* block = blocks + i;
        __blockBuffer_initBlock(block, blockBuffer->blockData + (i << bytePerBlockShift), INVALID_INDEX);

        linkedListNode_insertBack(&blockBuffer->LRU, &block->LRUnode);
    }

    SinglyLinkedList* chains = memory_allocate(sizeof(SinglyLinkedList) * chainNum);
    if (chains == NULL) {
        return RESULT_FAIL;
    }

    hashTable_initStruct(&blockBuffer->hashTable, chainNum, chains, hashTable_defaultHashFunc);
    blockBuffer->blockNum           = blockNum;

    return RESULT_SUCCESS;
}

void blockBuffer_clearStruct(BlockBuffer* blockBuffer) {
    Uintptr minOldBlockPtr = -1;
    LinkedListNode* current = linkedListNode_getNext(&blockBuffer->LRU);
    for (int i = 0; i < blockBuffer->blockNum; ++i) {
        minOldBlockPtr = algorithms_umin64(minOldBlockPtr, (Uintptr)HOST_POINTER(current, Block, LRUnode));
        current = linkedListNode_getNext(current);
    }

    memory_free((void*)minOldBlockPtr);
    memory_free(blockBuffer->hashTable.chains);
    memory_freeFrame(paging_convertAddressV2P(blockBuffer->blockData));

    memory_memset(blockBuffer, 0, sizeof(BlockBuffer));
}

Result blockBuffer_resize(BlockBuffer* blockBuffer, Size newBlockNum) {
    Block* newBlocks = memory_allocate(sizeof(Block) * newBlockNum);
    if (newBlocks == NULL) {
        return RESULT_FAIL;
    }

    LinkedListNode* current = linkedListNode_getNext(&blockBuffer->LRU);
    Uintptr minOldBlockPtr = -1;
    Size maxBlockNum = algorithms_umax64(blockBuffer->blockNum, newBlockNum);
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
                __blockBuffer_initBlock(newBlock, oldBlock->data, oldBlock->blockIndex);
            } else {
                __blockBuffer_initBlock(newBlock, NULL, INVALID_INDEX);
            }

            linkedListNode_insertFront(&blockBuffer->LRU, &newBlock->LRUnode);
        }

        if (oldBlock != NULL) {
            minOldBlockPtr = algorithms_umin64(minOldBlockPtr, (Uintptr)oldBlock);
            linkedListNode_delete(&oldBlock->LRUnode);
        }
    }

    memory_free((void*)minOldBlockPtr);
    blockBuffer->blockNum = newBlockNum;

    return RESULT_SUCCESS;
}

Block* blockBuffer_search(BlockBuffer* blockBuffer, Index64 blockIndex) {
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

Block* blockBuffer_pop(BlockBuffer* blockBuffer, Index64 blockIndex) {   //If blockIndex is INVALID_INDEX, it pops the tail of the LRU list
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

Result blockBuffer_push(BlockBuffer* blockBuffer, Index64 blockIndex, Block* block) {
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

static void __blockBuffer_initBlock(Block* block, void* data, Index64 blockIndex) {
    block->data         = data;
    block->blockIndex   = blockIndex;
    linkedListNode_initStruct(&block->LRUnode);
    hashChainNode_initStruct(&block->hashChainNode);
    block->flags        = EMPTY_FLAGS;
}