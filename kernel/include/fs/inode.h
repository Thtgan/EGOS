#if !defined(__FS_INODE_H)
#define __FS_INODE_H

typedef struct iNode iNode;
typedef struct iNodeOperations iNodeOperations;

#include<fs/superblock.h>
#include<kit/types.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

typedef struct iNode {
    Uint32                  signature;
#define INODE_SIGNATURE     0x120DE516
    Size                    sizeInBlock;
    SuperBlock*             superBlock;
    Uint32                  openCnt;
    iNodeOperations*        operations;
    HashChainNode           openedNode;
    SinglyLinkedListNode    mountNode;

    union {
        Object              specificInfo;
        Device*             device; //Only for when iNode is mapped to a device
    };
} iNode;

typedef struct iNodeOperations {
    Result (*translateBlockPos)(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);

    Result (*resize)(iNode* iNode, Size newSizeInByte);
} iNodeOperations;

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

#endif // __FS_INODE_H
