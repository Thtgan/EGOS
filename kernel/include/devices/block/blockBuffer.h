#if !defined(__DEVICES_BLOCK_BLOCKBUFFER_H)
#define __DEVICES_BLOCK_BLOCKBUFFER_H

typedef struct BlockBufferBlock BlockBufferBlock;
typedef struct BlockBuffer BlockBuffer;

#include<kit/bit.h>
#include<kit/types.h>
#include<structs/linkedList.h>
#include<structs/hashTable.h>

typedef struct BlockBufferBlock {
    void*           data;
    Index64         blockIndex;
    LinkedListNode  LRUnode;
    HashChainNode   hashChainNode;
#define BLOCK_BUFFER_BLOCK_FLAGS_PRESENT    FLAG8(0)
#define BLOCK_BUFFER_BLOCK_FLAGS_DIRTY      FLAG8(1)
    Flags8          flags;
} BlockBufferBlock;

#define BLOCK_BUFFER_DEFAULT_MAX_BLOCK_NUM  32
#define BLOCK_BUFFER_DEFAULT_HASH_SIZE      16

typedef struct BlockBuffer {
    Size            bytePerBlockShift;
    void*           blockData;
    LinkedList      LRU;
    HashTable       hashTable;
    Size            blockNum;
} BlockBuffer;

void blockBuffer_initStruct(BlockBuffer* blockBuffer, Size chainNum, Size blockNum, Size blockSizeShift);

void blockBuffer_clearStruct(BlockBuffer* blockBuffer);

void blockBuffer_resize(BlockBuffer* blockBuffer, Size newBlockNum);

BlockBufferBlock* blockBuffer_pop(BlockBuffer* blockBuffer, Index64 blockIndex);

void blockBuffer_push(BlockBuffer* blockBuffer, Index64 blockIndex, BlockBufferBlock* block);

#endif // __DEVICES_BLOCK_BLOCKBUFFER_H
