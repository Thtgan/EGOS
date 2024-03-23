#if !defined(__INODE_H)
#define __INODE_H

#include<fs/fsStructs.h>
#include<kit/types.h>
#include<structs/hashTable.h>

static inline Result iNode_rawTranslateBlockPos(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN) {
    return iNode->operations->translateBlockPos(iNode, vBlockIndex, n, pBlockRanges, rangeN);
}

static inline Result iNode_rawResize(iNode* iNode, Size newSizeInByte) {
    return iNode->operations->resize(iNode, newSizeInByte);
}

iNode* iNode_openFromOpened(HashTable* table, Index64 blockIndex);

Result iNode_addToOpened(HashTable* table, iNode* iNode, Index64 blockIndex);

Result iNode_removeFromOpened(HashTable* table, Index64 blockIndex);

iNode* iNode_open(SuperBlock* superBlock, fsEntryDesc* desc);

Result iNode_close(iNode* iNode, fsEntryDesc* desc);

#endif // __INODE_H
