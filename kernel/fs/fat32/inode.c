#include<fs/fat32/inode.h>

#include<algorithms.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/fat32.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

typedef struct {
    Index64 firstCluster;
} __FAT32iNodeInfo;

static Result __iNode_FAT32_mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);

static Result __iNode_FAT32_resize(iNode* iNode, Size newSizeInByte);

static iNodeOperations _FAT32iNodeOperations = {
    .translateBlockPos  = __iNode_FAT32_mapBlockPosition,
    .resize             = __iNode_FAT32_resize
};

Result FAT32_iNode_open(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc) {
    __FAT32iNodeInfo* iNodeInfo = memory_allocate(sizeof(__FAT32iNodeInfo));
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
    hashChainNode_initStruct(&iNode->openedNode);
    singlyLinkedListNode_initStruct(&iNode->mountNode);
    iNode->specificInfo     = (Object)iNodeInfo;

    return RESULT_SUCCESS;
}

Result FAT32_iNode_close(SuperBlock* superBlock, iNode* iNode) {
    memory_free((void*)iNode->specificInfo);
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

    __FAT32iNodeInfo* iNodeInfo = (__FAT32iNodeInfo*)iNode->specificInfo;

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

static Result __iNode_FAT32_resize(iNode* iNode, Size newSizeInByte) {
    FAT32info* info = (FAT32info*)iNode->superBlock->specificInfo;
    FAT32BPB* BPB = info->BPB;
    __FAT32iNodeInfo* iNodeInfo = (__FAT32iNodeInfo*)iNode->specificInfo;
    Size newSizeInCluster = DIVIDE_ROUND_UP(DIVIDE_ROUND_UP_SHIFT(newSizeInByte, iNode->superBlock->device->bytePerBlockShift), BPB->sectorPerCluster), oldSizeInCluster = DIVIDE_ROUND_UP(iNode->sizeInBlock, BPB->sectorPerCluster);

    if (newSizeInCluster < oldSizeInCluster) {
        Index32 tail = FAT32_cluster_get(info, iNodeInfo->firstCluster, newSizeInCluster - 1);
        if (FAT32_cluster_getType(info, tail) != FAT32_CLUSTER_TYPE_ALLOCATERD || PTR_TO_VALUE(32, info->FAT + tail) == FAT32_CLSUTER_END_OF_CHAIN) {
            return RESULT_FAIL;
        }

        Index32 cut = FAT32_cluster_cutChain(info, tail);
        FAT32_cluster_freeChain(info, cut);
    } else if (newSizeInCluster > oldSizeInCluster) {
        Index32 freeClusterChain = FAT32_cluster_allocChain(info, newSizeInCluster - oldSizeInCluster);
        Index32 tail = FAT32_cluster_get(info, iNodeInfo->firstCluster, iNode->sizeInBlock / BPB->sectorPerCluster - 1);

        if (FAT32_cluster_getType(info, tail) != FAT32_CLUSTER_TYPE_ALLOCATERD || FAT32_cluster_getType(info, freeClusterChain) != FAT32_CLUSTER_TYPE_ALLOCATERD || PTR_TO_VALUE(32, info->FAT + tail) != FAT32_CLSUTER_END_OF_CHAIN) {
            return RESULT_FAIL;
        }

        FAT32_cluster_insertChain(info, tail, freeClusterChain);
    }

    iNode->sizeInBlock = newSizeInCluster * BPB->sectorPerCluster;

    return RESULT_SUCCESS;
}