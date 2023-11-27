#if !defined(__INODE_H)
#define __INODE_H

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsPreDefines.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef struct {
    Result (*translateBlockPos)(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);
} iNodeOperations;

STRUCT_PRIVATE_DEFINE(iNode) {
    Uint32              signature;
#define INODE_SIGNATURE 0x120DE516
    Size                sizeInBlock;
    SuperBlock*         superBlock;
    Uint32              openCnt;
    iNodeOperations*    operations;
    HashChainNode       hashChainNode;

    Object              specificInfo;
};

static inline Result iNode_rawTranslateBlockPos(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN) {
    return iNode->operations->translateBlockPos(iNode, vBlockIndex, n, pBlockRanges, rangeN);
}

iNode* iNode_openFromOpened(HashTable* table, Index64 blockIndex);

Result iNode_addToOpened(HashTable* table, iNode* iNode, Index64 blockIndex);

Result iNode_removeFromOpened(HashTable* table, Index64 blockIndex);

iNode* iNode_open(SuperBlock* superBlock, FSentryDesc* desc);

Result iNode_close(iNode* iNode, FSentryDesc* desc);

#endif // __INODE_H
