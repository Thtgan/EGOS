#include<fs/fat32/inode.h>

#include<algorithms.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/fat32.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

typedef struct {
    Index64 firstCluster;
} __FAT32iNodeInfo;

static Result __fat32_iNode_mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);

static Result __fat32_iNode_resize(iNode* iNode, Size newSizeInByte);

static iNodeOperations _fat32_iNodeOperations = {
    .translateBlockPos  = __fat32_iNode_mapBlockPosition,
    .resize             = __fat32_iNode_resize
};

Result fat32_iNode_open(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc) {
    __FAT32iNodeInfo* iNodeInfo = memory_allocate(sizeof(__FAT32iNodeInfo));
    if (iNodeInfo == NULL) {
        return RESULT_ERROR;
    }

    FAT32info* info             = superBlock->specificInfo;
    FAT32BPB* BPB               = info->BPB;
    Device* superBlockDevice    = &superBlock->blockDevice->device;
    iNodeInfo->firstCluster     = DIVIDE_ROUND_DOWN(DIVIDE_ROUND_DOWN_SHIFT(desc->dataRange.begin, superBlockDevice->granularity) - info->dataBlockRange.begin, BPB->sectorPerCluster);

    iNode->signature            = INODE_SIGNATURE;
    iNode->sizeInBlock          = DIVIDE_ROUND_UP_SHIFT(desc->dataRange.begin + desc->dataRange.length, superBlockDevice->granularity) - DIVIDE_ROUND_DOWN_SHIFT(desc->dataRange.begin, superBlockDevice->granularity);
    iNode->superBlock           = superBlock;
    iNode->openCnt              = 1;
    iNode->operations           = &_fat32_iNodeOperations;
    hashChainNode_initStruct(&iNode->openedNode);
    singlyLinkedListNode_initStruct(&iNode->mountNode);
    iNode->specificInfo         = (Object)iNodeInfo;

    return RESULT_SUCCESS;
}

Result fat32_iNode_close(SuperBlock* superBlock, iNode* iNode) {
    memory_free((void*)iNode->specificInfo);
    return RESULT_SUCCESS;
}

static Result __fat32_iNode_mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN) {
    if (*n == 0) {
        return RESULT_SUCCESS;
    }

    SuperBlock* superBlock = iNode->superBlock;
    Index32 vBlockEnd = iNode->sizeInBlock;
    if (*vBlockIndex + *n > vBlockEnd) {
        return RESULT_ERROR;
    }

    FAT32info* info = (FAT32info*)superBlock->specificInfo;
    FAT32BPB* BPB = info->BPB;

    __FAT32iNodeInfo* iNodeInfo = (__FAT32iNodeInfo*)iNode->specificInfo;

    Index64 offsetInCluster = *vBlockIndex;
    Index32 current = iNodeInfo->firstCluster;
    if (fat32_getClusterType(info, current) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return RESULT_ERROR;
    }

    while (offsetInCluster >= BPB->sectorPerCluster) {
        current = fat32_getCluster(info, current, 1);

        if (fat32_getClusterType(info, current) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            return RESULT_ERROR;
        }

        offsetInCluster -= BPB->sectorPerCluster;
    }

    Size remain = *n;
    for (int i = 0; remain > 0 && i < rangeN; ++i) {
        Range* range = pBlockRanges + i;
        range->begin = info->dataBlockRange.begin + current * BPB->sectorPerCluster + offsetInCluster;
        range->length = 0;

        while (true) {
            Size remainInCluster = algorithms_min64(remain, BPB->sectorPerCluster - offsetInCluster);
            offsetInCluster = 0;
            range->length += remainInCluster;
            remain -= remainInCluster;

            if (remain == 0) {
                break;
            }

            Index32 next = fat32_getCluster(info, current, 1);
            if (fat32_getClusterType(info, next) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
                return RESULT_ERROR;
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

static Result __fat32_iNode_resize(iNode* iNode, Size newSizeInByte) {
    FAT32info* info = (FAT32info*)iNode->superBlock->specificInfo;
    FAT32BPB* BPB = info->BPB;
    __FAT32iNodeInfo* iNodeInfo = (__FAT32iNodeInfo*)iNode->specificInfo;
    Device* superBlockDevice = &iNode->superBlock->blockDevice->device;

    Size newSizeInCluster = DIVIDE_ROUND_UP(DIVIDE_ROUND_UP_SHIFT(newSizeInByte, superBlockDevice->granularity), BPB->sectorPerCluster), oldSizeInCluster = DIVIDE_ROUND_UP(iNode->sizeInBlock, BPB->sectorPerCluster);

    if (newSizeInCluster < oldSizeInCluster) {
        Index32 tail = fat32_getCluster(info, iNodeInfo->firstCluster, newSizeInCluster - 1);
        if (fat32_getClusterType(info, tail) != FAT32_CLUSTER_TYPE_ALLOCATERD || PTR_TO_VALUE(32, info->FAT + tail) == FAT32_CLSUTER_END_OF_CHAIN) {
            return RESULT_ERROR;
        }

        Index32 cut = fat32_cutClusterChain(info, tail);
        fat32_freeClusterChain(info, cut);
    } else if (newSizeInCluster > oldSizeInCluster) {
        Index32 freeClusterChain = fat32_allocateClusterChain(info, newSizeInCluster - oldSizeInCluster);
        Index32 tail = fat32_getCluster(info, iNodeInfo->firstCluster, iNode->sizeInBlock / BPB->sectorPerCluster - 1);

        if (fat32_getClusterType(info, tail) != FAT32_CLUSTER_TYPE_ALLOCATERD || fat32_getClusterType(info, freeClusterChain) != FAT32_CLUSTER_TYPE_ALLOCATERD || PTR_TO_VALUE(32, info->FAT + tail) != FAT32_CLSUTER_END_OF_CHAIN) {
            return RESULT_ERROR;
        }

        fat32_insertClusterChain(info, tail, freeClusterChain);
    }

    iNode->sizeInBlock = newSizeInCluster * BPB->sectorPerCluster;

    return RESULT_SUCCESS;
}