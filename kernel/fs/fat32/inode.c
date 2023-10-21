#include<fs/fat32/inode.h>

#include<algorithms.h>
#include<fs/fat32/fat32.h>
#include<fs/fileSystem.h>
#include<fs/fileSystemEntry.h>
#include<fs/inode.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<structs/hashTable.h>

static Result __FAT32mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);

static Result __FAT32resize(iNode* iNode, Size newBlockSize);

static iNodeOperations _FAT32iNodeOperations = {
    .mapBlockPosition   = __FAT32mapBlockPosition
};

Result FAT32openInode(SuperBlock* superBlock, iNode* iNode, FileSystemEntryDescriptor* entryDescripotor) {
    FAT32iNodeInfo* iNodeInfo = kMalloc(sizeof(FAT32iNodeInfo));
    if (iNodeInfo == NULL) {
        return RESULT_FAIL;
    }

    FAT32info* info         = superBlock->specificInfo;
    FAT32BPB* BPB           = info->BPB;
    BlockDevice* device     = superBlock->device;
    iNodeInfo->firstCluster = DIVIDE_ROUND_DOWN(DIVIDE_ROUND_DOWN_SHIFT(entryDescripotor->dataRange.begin, superBlock->device->bytePerBlockShift) - info->dataBlockRange.begin, BPB->sectorPerCluster);

    iNode->signature        = INODE_SIGNATURE;
    iNode->sizeInBlock      = DIVIDE_ROUND_UP_SHIFT(entryDescripotor->dataRange.begin + entryDescripotor->dataRange.length, device->bytePerBlockShift) - DIVIDE_ROUND_DOWN_SHIFT(entryDescripotor->dataRange.begin, device->bytePerBlockShift);
    iNode->superBlock       = superBlock;
    iNode->openCnt          = 0;
    iNode->operations       = &_FAT32iNodeOperations;
    initHashChainNode(&iNode->hashChainNode);
    iNode->specificInfo     = (Object)iNodeInfo;

    return RESULT_SUCCESS;
}

Result FAT32closeInode(SuperBlock* superBlock, iNode* iNode) {
    kFree((void*)iNode->specificInfo);
    return RESULT_SUCCESS;
}

static Result __FAT32mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN) {
    SuperBlock* superBlock = iNode->superBlock;
    BlockDevice* device = superBlock->device;
    Index32 vBlockEnd = iNode->sizeInBlock;
    if (*vBlockIndex + *n > vBlockEnd) {
        return RESULT_FAIL;
    }

    FAT32info* info = (FAT32info*)superBlock->specificInfo;
    FAT32BPB* BPB = info->BPB;

    FAT32iNodeInfo* iNodeInfo = (FAT32iNodeInfo*)iNode->specificInfo;

    Index64 offsetInCluster = *vBlockIndex;
    Index32 current = iNodeInfo->firstCluster;
    if (FAT32getClusterType(info, current) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return RESULT_FAIL;
    }

    while (offsetInCluster >= BPB->sectorPerCluster) {
        current = FAT32getCluster(info, current, 1);

        if (FAT32getClusterType(info, current) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            return RESULT_FAIL;
        }

        offsetInCluster -= BPB->sectorPerCluster;
    }

    Size remain = *n;
    for (int i = 0; remain > 0 && i < rangeN; ++i) {
        Range* range = pBlockRanges + i;
        range->begin = info->dataBlockRange.begin + current * BPB->sectorPerCluster + offsetInCluster;
        range->length = 0;

        while (true) {
            Size remainInCluster = min64(remain, BPB->sectorPerCluster - offsetInCluster);
            offsetInCluster = 0;
            range->length += remainInCluster;
            remain -= remainInCluster;

            if (remain == 0) {
                break;
            }

            Index32 next = FAT32getCluster(info, current, 1);
            if (FAT32getClusterType(info, next) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
                return RESULT_FAIL;
            }
            current = next;

            if (next != current + 1) {
                break;
            }
        }
    }

    *vBlockIndex += (*n - remain);
    *n = remain;

    return remain > 0 ? RESULT_CONTINUE : RESULT_SUCCESS;
}
