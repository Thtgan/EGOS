#if !defined(__INODE_H)
#define __INODE_H

#include<devices/block/blockDevice.h>
#include<fs/fileSystem.h>
#include<fs/fileSystemEntry.h>
#include<fs/fsPreDefines.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef struct {
    Result (*mapBlockPosition)(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);
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

static inline Result rawInodeMapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN) {
    return iNode->operations->mapBlockPosition(iNode, vBlockIndex, n, pBlockRanges, rangeN);
}

iNode* openInodeBuffered(SuperBlock* superBlock, FileSystemEntryDescriptor* entryDescriptor);

Result closeInodeBuffered(iNode* iNode, FileSystemEntryDescriptor* entryDescriptor);

#endif // __INODE_H
