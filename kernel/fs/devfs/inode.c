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
#include<debug.h>
#include<error.h>

typedef struct {
    Index8  firstBlock;
} __DEVFSiNodeInfo;

static bool __devfs_iNode_mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);

static void __devfs_iNode_resize(iNode* iNode, Size newSizeInByte);

static iNodeOperations _devfs_iNodeOperations = {
    .translateBlockPos  = __devfs_iNode_mapBlockPosition,
    .resize             = __devfs_iNode_resize
};

void devfs_iNode_open(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc) {
    __DEVFSiNodeInfo* iNodeInfo = NULL;
    iNodeInfo = memory_allocate(sizeof(__DEVFSiNodeInfo));
    if (iNodeInfo == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Size granularity = superBlockBlockDevice->device.granularity;
    if (desc->type == FS_ENTRY_TYPE_DEVICE) {
        Device* device = device_getDevice(desc->device);
        if (device == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
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
    iNode->iNodeID          = iNode_generateID(desc);
    iNode->superBlock       = superBlock;
    iNode->openCnt          = 1;
    iNode->operations       = &_devfs_iNodeOperations;
    hashChainNode_initStruct(&iNode->openedNode);
    singlyLinkedListNode_initStruct(&iNode->mountNode);

    return;
    ERROR_FINAL_BEGIN(0);
    if (iNodeInfo != NULL) {
        memory_free(iNodeInfo);
    }
}

void devfs_iNode_close(SuperBlock* superBlock, iNode* iNode) {
    memory_free((void*)iNode->specificInfo);
}

//TODO: Seems it needs rework...
static bool __devfs_iNode_mapBlockPosition(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN) {
    DEBUG_ASSERT_SILENT(vBlockIndex != NULL && n != NULL && pBlockRanges != NULL);
    if (*n == 0) {
        return true;
    }

    Index32 vBlockEnd = iNode->sizeInBlock;
    if (*vBlockIndex + *n > vBlockEnd) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    __DEVFSiNodeInfo* iNodeInfo = (__DEVFSiNodeInfo*)iNode->specificInfo;

    Index64 offsetInCluster = *vBlockIndex;
    Index32 current = iNodeInfo->firstBlock;
    if (current == INVALID_INDEX) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    DevFSblockChains* blockChains = (DevFSblockChains*)iNode->superBlock->specificInfo;
    current = devfs_blockChain_get(blockChains, current, offsetInCluster);
    if (current == INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
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
                ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
            }
            current = next;

            if (next != current + 1) {
                break;
            }
        }
    }

    *vBlockIndex += (*n - remain);
    *n = remain;

    ERROR_FINAL_BEGIN(0);
    return false;
}

static void __devfs_iNode_resize(iNode* iNode, Size newSizeInByte) {
    Index32 freeBlockChain = INVALID_INDEX;

    DevFSblockChains* blockChains = (DevFSblockChains*)iNode->superBlock->specificInfo;
    __DEVFSiNodeInfo* iNodeInfo = (__DEVFSiNodeInfo*)iNode->specificInfo;
    Size newSizeInBlock = DIVIDE_ROUND_UP_SHIFT(newSizeInByte, iNode->superBlock->blockDevice->device.granularity), oldSizeInBlock = iNode->sizeInBlock;

    if (newSizeInBlock < oldSizeInBlock) {
        Index32 tail = devfs_blockChain_get(blockChains, iNodeInfo->firstBlock, newSizeInBlock - 1);
        if (tail == INVALID_INDEX) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        Index32 cut = devfs_blockChain_cutChain(blockChains, tail);
        devfs_blockChain_freeChain(blockChains, cut);
    } else if (newSizeInBlock > oldSizeInBlock) {
        freeBlockChain = devfs_blockChain_allocChain(blockChains, newSizeInBlock - oldSizeInBlock);
        if (freeBlockChain == INVALID_INDEX) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        Index32 tail = devfs_blockChain_get(blockChains, iNodeInfo->firstBlock, iNode->sizeInBlock - 1);
        if (freeBlockChain == INVALID_INDEX) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        devfs_blockChain_insertChain(blockChains, tail, freeBlockChain);
    }

    iNode->sizeInBlock = newSizeInBlock;

    return;
    ERROR_FINAL_BEGIN(0);
    if (freeBlockChain != INVALID_INDEX) {
        devfs_blockChain_freeChain(blockChains, freeBlockChain);
    }
}