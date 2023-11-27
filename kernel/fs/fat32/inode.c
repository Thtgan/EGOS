#include<fs/fat32/inode.h>

#include<algorithms.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/fat32.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<structs/hashTable.h>

static Result __iNode_FAT32_mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);

static iNodeOperations _FAT32iNodeOperations = {
    .translateBlockPos   = __iNode_FAT32_mapBlockPosition
};

Result FAT32_iNode_open(SuperBlock* superBlock, iNode* iNode, FSentryDesc* desc) {
    FAT32iNodeInfo* iNodeInfo = kMalloc(sizeof(FAT32iNodeInfo));
    if (iNodeInfo == NULL) {
        return RESULT_FAIL;
    }

    FAT32info* info         = superBlock->specificInfo;
    FAT32BPB* BPB           = info->BPB;
    BlockDevice* device     = superBlock->device;
    iNodeInfo->firstCluster = DIVIDE_ROUND_DOWN(DIVIDE_ROUND_DOWN_SHIFT(desc->dataRange.begin, superBlock->device->bytePerBlockShift) - info->dataBlockRange.begin, BPB->sectorPerCluster);

    iNode->signature        = INODE_SIGNATURE;
    iNode->sizeInBlock      = DIVIDE_ROUND_UP_SHIFT(desc->dataRange.begin + desc->dataRange.length, device->bytePerBlockShift) - DIVIDE_ROUND_DOWN_SHIFT(desc->dataRange.begin, device->bytePerBlockShift);
    iNode->superBlock       = superBlock;
    iNode->openCnt          = 1;
    iNode->operations       = &_FAT32iNodeOperations;
    hashChainNode_initStruct(&iNode->hashChainNode);
    iNode->specificInfo     = (Object)iNodeInfo;

    return RESULT_SUCCESS;
}

Result FAT32_iNode_close(SuperBlock* superBlock, iNode* iNode) {
    kFree((void*)iNode->specificInfo);
    return RESULT_SUCCESS;
}

static Result __iNode_FAT32_mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN) {
    if (*n == 0) {
        return RESULT_SUCCESS;
    }

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
    if (FAT32_cluster_getType(info, current) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return RESULT_FAIL;
    }

    while (offsetInCluster >= BPB->sectorPerCluster) {
        current = FAT32_cluster_get(info, current, 1);

        if (FAT32_cluster_getType(info, current) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
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

            Index32 next = FAT32_cluster_get(info, current, 1);
            if (FAT32_cluster_getType(info, next) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
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

    return RESULT_CONTINUE;
}
