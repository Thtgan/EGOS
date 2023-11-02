#if !defined(__BLOCK_BUFFER_H)
#define __BLOCK_BUFFER_H

#include<kit/bit.h>
#include<kit/types.h>
#include<structs/linkedList.h>
#include<structs/hashTable.h>

typedef struct {
    void*           data;
    Index64         blockIndex;
    LinkedListNode  LRUnode;
    HashChainNode   hashChainNode;
#define BLOCK_BUFFER_BLOCK_FLAGS_PRESENT    FLAG8(0)
#define BLOCK_BUFFER_BLOCK_FLAGS_DIRTY      FLAG8(1)
    Flags8          flags;
} Block;

#define BLOCK_BUFFER_DEFAULT_MAX_BLOCK_NUM  32
#define BLOCK_BUFFER_DEFAULT_HASH_SIZE      16

typedef struct {
    Size            bytePerBlockShift;
    void*           blockData;
    LinkedList      LRU;
    HashTable       hashTable;
    Size            blockNum;
} BlockBuffer;

Result initBlockBuffer(BlockBuffer* blockBuffer, Size chainNum, Size blockNum, Size blockSizeShift);

void releaseBlockBuffer(BlockBuffer* blockBuffer);

Result resizeBlockBuffer(BlockBuffer* blockBuffer, Size newBlockNum);

Block* blockBufferPop(BlockBuffer* blockBuffer, Index64 blockIndex);

Result blockBufferPush(BlockBuffer* blockBuffer, Index64 blockIndex, Block* block);

#endif // __BLOCK_BUFFER_H
