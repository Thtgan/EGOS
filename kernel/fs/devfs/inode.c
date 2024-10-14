#include<fs/devfs/inode.h>

#include<devices/block/blockDevice.h>
#include<fs/devfs/blockChain.h>
#include<fs/devfs/devfs.h>
#include<fs/devfs/fsEntry.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>

typedef struct {
    Index8  firstBlock;
} __DEVFSiNodeInfo;

static Result __devfs_iNode_mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);

static Result __devfs_iNode_resize(iNode* iNode, Size newSizeInByte);

static iNodeOperations _devfs_iNodeOperations = {
    .translateBlockPos  = __devfs_iNode_mapBlockPosition,
    .resize             = __devfs_iNode_resize
};

Result devfs_iNode_open(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc) {
    __DEVFSiNodeInfo* iNodeInfo = memory_allocate(sizeof(__DEVFSiNodeInfo));
    if (iNodeInfo == NULL) {
        return RESULT_ERROR;
    }

    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Size granularity = superBlockBlockDevice->device.granularity;
    if (desc->type == FS_ENTRY_TYPE_DEVICE) {
        Device* device = device_getDevice(desc->device);
        if (device == NULL) {
            return RESULT_ERROR;
        }

        iNode->sizeInBlock      = INFINITE;
        iNode->device           = device;
    } else {
        DevFSblockChains* chains = &((DEVFSspecificInfo*)superBlock->specificInfo)->chains;
        iNodeInfo->firstBlock   = DIVIDE_ROUND_DOWN_SHIFT(desc->dataRange.begin, granularity);
        Uint64 sizeInBlock = devfs_blockChain_getChainLength(chains, iNodeInfo->firstBlock);
        iNode->sizeInBlock      = sizeInBlock;
        iNode->specificInfo     = (Object)iNodeInfo;
    }

    iNode->signature        = INODE_SIGNATURE;
    iNode->superBlock       = superBlock;
    iNode->openCnt          = 1;
    iNode->operations       = &_devfs_iNodeOperations;
    hashChainNode_initStruct(&iNode->openedNode);
    singlyLinkedListNode_initStruct(&iNode->mountNode);

    return RESULT_SUCCESS;
}

Result devfs_iNode_close(SuperBlock* superBlock, iNode* iNode) {
    memory_free((void*)iNode->specificInfo);
    return RESULT_SUCCESS;
}

static Result __devfs_iNode_mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN) {
    if (*n == 0) {
        return RESULT_SUCCESS;
    }

    Index32 vBlockEnd = iNode->sizeInBlock;
    if (*vBlockIndex + *n > vBlockEnd) {
        return RESULT_ERROR;
    }

    __DEVFSiNodeInfo* iNodeInfo = (__DEVFSiNodeInfo*)iNode->specificInfo;

    Index64 offsetInCluster = *vBlockIndex;
    Index32 current = iNodeInfo->firstBlock;
    if (current == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    DevFSblockChains* blockChains = (DevFSblockChains*)iNode->superBlock->specificInfo;
    current = devfs_blockChain_get(blockChains, current, offsetInCluster);
    if (current == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    Size remain = *n;
    for (int i = 0; remain > 0 && i < rangeN; ++i) {
        Range* range = pBlockRanges + i;
        range->begin = current;
        range->length = 0;

        while (true) {
            ++range->length;
            --remain;

            if (remain == 0) {
                break;
            }

            Index32 next = blockChains->nextBlocks[current];
            if (next == INVALID_INDEX) {
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

static Result __devfs_iNode_resize(iNode* iNode, Size newSizeInByte) {
    DevFSblockChains* blockChains = (DevFSblockChains*)iNode->superBlock->specificInfo;
    __DEVFSiNodeInfo* iNodeInfo = (__DEVFSiNodeInfo*)iNode->specificInfo;
    Size newSizeInBlock = DIVIDE_ROUND_UP_SHIFT(newSizeInByte, iNode->superBlock->blockDevice->device.granularity), oldSizeInBlock = iNode->sizeInBlock;

    if (newSizeInBlock < oldSizeInBlock) {
        Index32 tail = devfs_blockChain_get(blockChains, iNodeInfo->firstBlock, newSizeInBlock - 1);
        if (tail == INVALID_INDEX) {
            return RESULT_ERROR;
        }

        Index32 cut = devfs_blockChain_cutChain(blockChains, tail);
        devfs_blockChain_freeChain(blockChains, cut);
    } else if (newSizeInBlock > oldSizeInBlock) {
        Index32 freeBlockChain = devfs_blockChain_allocChain(blockChains, newSizeInBlock - oldSizeInBlock);
        Index32 tail = devfs_blockChain_get(blockChains, iNodeInfo->firstBlock, iNode->sizeInBlock - 1);

        if (tail == INVALID_INDEX || freeBlockChain == INVALID_INDEX) {
            return RESULT_ERROR;
        }

        devfs_blockChain_insertChain(blockChains, tail, freeBlockChain);
    }

    iNode->sizeInBlock = newSizeInBlock;

    return RESULT_SUCCESS;
}