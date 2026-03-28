#include<devices/blockBuffer.h>

#include<algorithms.h>
#include<lib/errorPosix.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<structs/linkedList.h>
#include<structs/hashTable.h>
#include<system/pageTable.h>

static void __blockBuffer_initBlock(BlockBufferBlock* block, void* data, Index64 blockIndex);

void blockBuffer_initStruct(BlockBuffer* blockBuffer, Size chainNum, Size blockNum, Size bytePerBlockShift) {
    blockBuffer->bytePerBlockShift  = bytePerBlockShift;
    void* blockData = mm_allocate(blockNum << bytePerBlockShift);
    ERROR_THROW_NEW_IF(blockData == NULL, ERROR_OUT_OF_MEMORY, error_out);
    blockBuffer->blockData          = blockData;

    linkedList_initStruct(&blockBuffer->LRU);

    BlockBufferBlock* blocks = mm_allocate(sizeof(BlockBufferBlock) * blockNum);
    ERROR_THROW_NEW_IF(blocks == NULL, ERROR_OUT_OF_MEMORY, error_out);

    for (int i = 0; i < blockNum; ++i) {
        BlockBufferBlock* block = blocks + i;
        __blockBuffer_initBlock(block, blockBuffer->blockData + (i << bytePerBlockShift), INVALID_INDEX64);

        linkedListNode_insertBack(&blockBuffer->LRU, &block->LRUnode);
    }

    SinglyLinkedList* chains = mm_allocate(sizeof(SinglyLinkedList) * chainNum);
    ERROR_THROW_NEW_IF(chains == NULL, ERROR_OUT_OF_MEMORY, error_out);

    hashTable_initStruct(&blockBuffer->hashTable, chainNum, chains, hashTable_defaultHashFunc);
    blockBuffer->blockNum           = blockNum;

    return;
error_out:
    if (blockData != NULL) {
        mm_free(blockData);
    }

    if (blocks != NULL) {
        mm_free(blocks);
    }

    if (chains != NULL) {
        mm_free(chains);
    }
}

void blockBuffer_clearStruct(BlockBuffer* blockBuffer) {
    Uintptr minOldBlockPtr = -1;
    LinkedListNode* current = linkedListNode_getNext(&blockBuffer->LRU);
    for (int i = 0; i < blockBuffer->blockNum; ++i) {
        minOldBlockPtr = algorithms_umin64(minOldBlockPtr, (Uintptr)HOST_POINTER(current, BlockBufferBlock, LRUnode));
        current = linkedListNode_getNext(current);
    }

    mm_free((void*)minOldBlockPtr);
    mm_free(blockBuffer->hashTable.chains);
    mm_free(blockBuffer->blockData);

    memory_memset(blockBuffer, 0, sizeof(BlockBuffer));
}

void blockBuffer_resize(BlockBuffer* blockBuffer, Size newBlockNum) {
    BlockBufferBlock* newBlocks = mm_allocate(sizeof(BlockBufferBlock) * newBlockNum);
    ERROR_THROW_NEW_IF(newBlocks == NULL, ERROR_OUT_OF_MEMORY, error_out);

    LinkedListNode* current = linkedListNode_getNext(&blockBuffer->LRU);
    Uintptr minOldBlockPtr = -1;
    Size maxBlockNum = algorithms_umax64(blockBuffer->blockNum, newBlockNum);
    for (int i = 0; i < maxBlockNum; ++i) {
        BlockBufferBlock* oldBlock = NULL, * newBlock = NULL;
        if (i < blockBuffer->blockNum) {
            oldBlock = HOST_POINTER(linkedListNode_getNext(&blockBuffer->LRU), BlockBufferBlock, LRUnode);
        }

        if (i < newBlockNum) {
            newBlock = newBlocks + i;
        }

        if (newBlock != NULL) {
            if (oldBlock != NULL) {
                __blockBuffer_initBlock(newBlock, oldBlock->data, oldBlock->blockIndex);
            } else {
                __blockBuffer_initBlock(newBlock, NULL, INVALID_INDEX64);
            }

            linkedListNode_insertFront(&blockBuffer->LRU, &newBlock->LRUnode);
        }

        if (oldBlock != NULL) {
            minOldBlockPtr = algorithms_umin64(minOldBlockPtr, (Uintptr)oldBlock);
            linkedListNode_delete(&oldBlock->LRUnode);
        }
    }

    mm_free((void*)minOldBlockPtr);
    blockBuffer->blockNum = newBlockNum;

    return;
error_out:
}

BlockBufferBlock* blockBuffer_search(BlockBuffer* blockBuffer, Index64 blockIndex) {
    HashChainNode* found = hashTable_find(&blockBuffer->hashTable, blockIndex);
    ERROR_THROW_NEW_IF(found == NULL, ERROR_NOT_FOUND, error_out);

    BlockBufferBlock* block = HOST_POINTER(found, BlockBufferBlock, hashChainNode);
    linkedListNode_delete(&block->LRUnode);
    linkedListNode_initStruct(&block->LRUnode);
    linkedListNode_insertBack(&blockBuffer->LRU, &block->LRUnode);

    return block;
error_out:
    return NULL;
}

BlockBufferBlock* blockBuffer_pop(BlockBuffer* blockBuffer, Index64 blockIndex) {   //If blockIndex is INVALID_INDEX, it pops the tail of the LRU list
    HashChainNode* found = hashTable_find(&blockBuffer->hashTable, blockIndex);

    BlockBufferBlock* block;
    if (found == NULL) {
        block = HOST_POINTER(linkedListNode_getPrev(&blockBuffer->LRU), BlockBufferBlock, LRUnode);
    } else {
        block = HOST_POINTER(found, BlockBufferBlock, hashChainNode);
    }

    if (block->blockIndex != INVALID_INDEX64) {
        HashChainNode* deleted = hashTable_delete(&blockBuffer->hashTable, block->blockIndex);
        ERROR_THROW_NEW_IF(deleted == NULL, ERROR_INVALID_STATE, error_out);
    }

    linkedListNode_delete(&block->LRUnode);

    hashChainNode_initStruct(&block->hashChainNode);
    linkedListNode_initStruct(&block->LRUnode);

    if (block->blockIndex != blockIndex) {
        CLEAR_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT);
    }

    return block;
error_out:
    return NULL;
}

void blockBuffer_push(BlockBuffer* blockBuffer, Index64 blockIndex, BlockBufferBlock* block) {
    block->blockIndex = blockIndex;
    if (blockIndex != INVALID_INDEX64) {
        hashTable_insert(&blockBuffer->hashTable, block->blockIndex, &block->hashChainNode);
        CHECK_ERROR(error_out);
    }

    linkedListNode_insertBack(&blockBuffer->LRU, &block->LRUnode);
    SET_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT);

    return;
error_out:
}

static void __blockBuffer_initBlock(BlockBufferBlock* block, void* data, Index64 blockIndex) {
    block->data         = data;
    block->blockIndex   = blockIndex;
    linkedListNode_initStruct(&block->LRUnode);
    hashChainNode_initStruct(&block->hashChainNode);
    block->flags        = EMPTY_FLAGS;
}