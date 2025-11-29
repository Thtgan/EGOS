#include<fs/ext2/ext2.h>

#include<devices/blockDevice.h>
#include<devices/device.h>
#include<fs/ext2/blockGroup.h>
#include<fs/ext2/vnode.h>
#include<kit/util.h>
#include<memory/mm.h>
#include<memory/memory.h>
#include<structs/refCounter.h>
#include<algorithms.h>
#include<error.h>

static vNode* __ext2_fscore_openVnode(FScore* fscore, fsNode* node);

static void __ext2_fscore_closeVnode(FScore* fscore, vNode* vnode);

static fsEntry* __ext2_fscore_openFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags);

#define __EXT2_SUPERBLOCK_OFFSET    1024

static ConstCstring __ext2_name = "EXT2";
static FScoreOperations __ext2_fscoreOperations = {
    .openVnode      = __ext2_fscore_openVnode,
    .closeVnode     = __ext2_fscore_closeVnode,
    .sync           = NULL,
    .openFSentry    = __ext2_fscore_openFSentry,
    .closeFSentry   = fscore_genericCloseFSentry,
    .mount          = fscore_genericMount,
    .unmount        = fscore_genericUnmount
};

static fsEntryOperations _ext2_fsEntryOperations = {
    .seek           = fsEntry_genericSeek,
    .read           = fsEntry_genericRead,
    .write          = fsEntry_genericWrite
};

Index32 ext2fscore_allocateBlock(EXT2fscore* fscore, Index32 preferredBlockGroup) {
    Index32 ret = INVALID_INDEX32;
    if (preferredBlockGroup < fscore->blockGroupNum && (ret = ext2blockGroupDescriptor_allocateBlock(&fscore->blockGroupTables[preferredBlockGroup], fscore)) != INVALID_INDEX32) {
        return ret;
    }

    for (int i = 0; i < fscore->blockGroupNum; ++i) {
        if (i == preferredBlockGroup) {
            continue;
        }

        if ((ret = ext2blockGroupDescriptor_allocateBlock(&fscore->blockGroupTables[i], fscore)) != INVALID_INDEX32) {
            break;
        }
    }

    return ret;
}

void ext2fscore_freeBlock(EXT2fscore* fscore, Index32 index) {
    EXT2SuperBlock* superblock = fscore->superBlock;
    Index32 blockGroupIndex = index / superblock->blockGroupBlockNum;
    DEBUG_ASSERT_SILENT(blockGroupIndex < fscore->blockGroupNum);
    Index32 inBlockIndex = index % superblock->blockGroupBlockNum;

    ext2blockGroupDescriptor_freeBlock(&fscore->blockGroupTables[blockGroupIndex], fscore, inBlockIndex);
}

Index32 ext2fscore_allocateInode(EXT2fscore* fscore, Index32 preferredBlockGroup, bool isDirectory) {
    Index32 ret = INVALID_INDEX32;
    if (preferredBlockGroup < fscore->blockGroupNum && (ret = ext2blockGroupDescriptor_allocateInode(&fscore->blockGroupTables[preferredBlockGroup], fscore, isDirectory)) != INVALID_INDEX32) {
        return ret;
    }

    for (int i = 0; i < fscore->blockGroupNum; ++i) {
        if (i == preferredBlockGroup) {
            continue;
        }

        if ((ret = ext2blockGroupDescriptor_allocateInode(&fscore->blockGroupTables[i], fscore, isDirectory)) != INVALID_INDEX32) {
            break;
        }
    }

    return ret;
}

void ext2fscore_freeInode(EXT2fscore* fscore, Index32 index) {
    EXT2SuperBlock* superblock = fscore->superBlock;
    Index32 blockGroupIndex = index / superblock->blockGroupInodeNum;
    DEBUG_ASSERT_SILENT(blockGroupIndex < fscore->blockGroupNum);
    Index32 inBlockIndex = index % superblock->blockGroupInodeNum;

    ext2blockGroupDescriptor_freeInode(&fscore->blockGroupTables[blockGroupIndex], fscore, inBlockIndex);
}

void ext2_init() {
    
}

bool ext2_checkType(BlockDevice* blockDevice) {
    if (blockDevice == NULL) {
        return false;
    }

    Device* device = &blockDevice->device;
    Size deviceBlockSize = POWER_2(device->granularity);
    Uint8 superBlockBuffer[algorithms_umax64(deviceBlockSize, sizeof(EXT2SuperBlock))];

    DEBUG_ASSERT_SILENT(__EXT2_SUPERBLOCK_OFFSET % deviceBlockSize == 0);
    Index64 superBlockIndex = __EXT2_SUPERBLOCK_OFFSET / deviceBlockSize;
    Size superBlockN = DIVIDE_ROUND_UP(sizeof(EXT2SuperBlock), deviceBlockSize);

    blockDevice_readBlocks(blockDevice, superBlockIndex, (void*)superBlockBuffer, superBlockN);
    ERROR_GOTO_IF_ERROR(0);

    EXT2SuperBlock* superBlock = (EXT2SuperBlock*)superBlockBuffer;
    bool ret = superBlock->signature == EXT2_SUPERBLOCK_IN_STORAGE_SIGNATURE && (EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superBlock->blockSizeShift) % deviceBlockSize) == 0;
    return ret;
    ERROR_FINAL_BEGIN(0);
    return false;
}

#define __FS_EXT2_ROOT_INODE_ID 2

// #define __FS_EXT2_FSCORE_HASH_BUCKET    16
// #define __FS_EXT2_BATCH_ALLOCATE_SIZE   BATCH_ALLOCATE_SIZE((EXT2fscore, 1), (EXT2SuperBlock, 1), (SinglyLinkedList, __FS_EXT2_FSCORE_HASH_BUCKET))
#define __FS_EXT2_BATCH_ALLOCATE_SIZE   BATCH_ALLOCATE_SIZE((EXT2fscore, 1), (EXT2SuperBlock, 1))

void ext2_open(FS* fs, BlockDevice* blockDevice) {
    void* batchAllocated = NULL;
    Device* device = &blockDevice->device;
    Size deviceBlockSize = POWER_2(device->granularity);
    Uint8 superBlockBuffer[algorithms_umax64(deviceBlockSize, sizeof(EXT2SuperBlock))];

    batchAllocated = mm_allocate(__FS_EXT2_BATCH_ALLOCATE_SIZE);
    if (batchAllocated == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (EXT2fscore, ext2fscore, 1),
        (EXT2SuperBlock, ext2SuperBlock, 1)
        // (EXT2SuperBlock, ext2SuperBlock, 1),
        // (SinglyLinkedList, openedVnodeChains, __FS_EXT2_FSCORE_HASH_BUCKET)
    );

    Index64 superBlockIndex = __EXT2_SUPERBLOCK_OFFSET / deviceBlockSize;
    Size superBlockN = DIVIDE_ROUND_UP(sizeof(EXT2SuperBlock), deviceBlockSize);
    blockDevice_readBlocks(blockDevice, superBlockIndex, (void*)superBlockBuffer, superBlockN);
    ERROR_GOTO_IF_ERROR(0);
    memory_memcpy(ext2SuperBlock, superBlockBuffer, sizeof(EXT2SuperBlock));
    ext2fscore->superBlock = ext2SuperBlock;

    Size blockGroupNum = ext2SuperBlock_getBlockGroupNum(ext2SuperBlock);
    ext2fscore->blockGroupNum = blockGroupNum;
    ext2fscore->blockGroupTables = mm_allocate(blockGroupNum * sizeof(EXT2blockGroupDescriptor));   //TODO: Merge this allocation

    Size blockGroupDeviceBlockNum = DIVIDE_ROUND_UP(blockGroupNum * sizeof(EXT2blockGroupDescriptor), deviceBlockSize);
    Uint8* deviceBlockBuffer = superBlockBuffer, * currentPointer = (Uint8*)ext2fscore->blockGroupTables;
    Index64 currentDeviceBlockIndex = ext2SuperBlock_blockIndexFS2device(ext2SuperBlock, blockDevice->device.granularity, ext2SuperBlock->blockSizeShift == 0 ? 2 : 1, 0);
    Size remainingByteN = blockGroupNum * sizeof(EXT2blockGroupDescriptor);
    for (int i = 0; i < blockGroupDeviceBlockNum; ++i) {
        blockDevice_readBlocks(blockDevice, currentDeviceBlockIndex, (void*)deviceBlockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        Size copyN = algorithms_umin64(remainingByteN, deviceBlockSize);
        memory_memcpy(currentPointer, deviceBlockBuffer, copyN);
        remainingByteN -= copyN;
        currentPointer += copyN;
        ++currentDeviceBlockIndex;
    }

    FScoreInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__ext2_fscoreOperations,
        .rootVnodeID        = __FS_EXT2_ROOT_INODE_ID,
        .rootFSnodePointsTo = 0
        // .openedVnodeBucket  = __FS_EXT2_FSCORE_HASH_BUCKET,
        // .openedVnodeChains  = openedVnodeChains
    };

    fscore_initStruct(&ext2fscore->fscore, &args);
    ERROR_GOTO_IF_ERROR(0);
    
    fs->name    = __ext2_name;
    fs->type    = FS_TYPE_EXT2;
    fs->fscore  = &ext2fscore->fscore;
    
    return;
    ERROR_FINAL_BEGIN(0);
    
    if (ext2fscore->blockGroupTables) {
        mm_free(ext2fscore->blockGroupTables);
    }

    if (batchAllocated != NULL) {
        mm_free(batchAllocated);
    }
}

void ext2_close(FS* fs) {
    FScore* fscore = fs->fscore;
    EXT2fscore* ext2fscore = HOST_POINTER(fscore, EXT2fscore, fscore);

    fscore_rawSync(fscore);
    ERROR_GOTO_IF_ERROR(0);

    void* batchAllocated = ext2fscore; //TODO: Ugly code
    memory_memset(batchAllocated, 0, __FS_EXT2_BATCH_ALLOCATE_SIZE);
    mm_free(batchAllocated);

    return;
    ERROR_FINAL_BEGIN(0);
}

static vNode* __ext2_fscore_openVnode(FScore* fscore, fsNode* node) {
    EXT2vnode* ext2vnode = NULL;
    EXT2fscore* ext2fscore = HOST_POINTER(fscore, EXT2fscore, fscore);

    Index32 inodeID = node->entry.vnodeID;
    EXT2SuperBlock* superBlock = ext2fscore->superBlock;
    Index32 blockGroupID = ext2SuperBlock_inodeID2BlockGroupIndex(superBlock, inodeID);

    DEBUG_ASSERT_SILENT(blockGroupID < ext2fscore->blockGroupNum);

    EXT2blockGroupDescriptor* desc = &ext2fscore->blockGroupTables[blockGroupID];
    
    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superBlock->blockSizeShift);
    Index32 inodeIndexInBlockGroup = (inodeID - 1) % superBlock->blockGroupInodeNum;
    Index32 inodeBlockIndex = desc->inodeTableIndex + (inodeIndexInBlockGroup * superBlock->iNodeSizeInByte) / blockSize;
    Index32 inodeBlockOffset = (inodeIndexInBlockGroup * superBlock->iNodeSizeInByte) % blockSize;

    Size granularity = fscore->blockDevice->device.granularity;
    Size deviceBlockSize = POWER_2(granularity);
    Index32 inodeDeviceBlockIndex = ext2SuperBlock_blockIndexFS2device(superBlock, granularity, inodeBlockIndex, inodeBlockOffset);
    Index32 inodeDeviceBlockOffset = ext2SuperBlock_blockOffsetFS2device(superBlock, granularity, inodeBlockIndex, inodeBlockOffset);

    DEBUG_ASSERT_SILENT(inodeDeviceBlockOffset + superBlock->iNodeSizeInByte <= deviceBlockSize);

    Uint8 deviceBlockBuffer[deviceBlockSize];
    blockDevice_readBlocks(fscore->blockDevice, inodeDeviceBlockIndex, deviceBlockBuffer, 1);
    ERROR_GOTO_IF_ERROR(0);

    ext2vnode = mm_allocate(sizeof(EXT2vnode));
    if (ext2vnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    EXT2inode* inode = &ext2vnode->inode;

    memory_memcpy(inode, deviceBlockBuffer + inodeDeviceBlockOffset, sizeof(EXT2inode));

    vNode* vnode = &ext2vnode->vnode;
    DirectoryEntry* nodeEntry = &node->entry;

    vNodeInitArgs args = {
        .vnodeID        = node->entry.vnodeID,
        .tokenSpaceSize = inode->sectorCnt * deviceBlockSize,
        .fscore         = fscore,
        .operations     = ext2_vNode_getOperations(),
        .fsNode         = node,
        .deviceID       = INVALID_ID
    };

    if (nodeEntry->type == FS_ENTRY_TYPE_FILE) {
        args.size       = nodeEntry->size;
        DEBUG_ASSERT_SILENT(args.size == (((Size)inode->h32Size << 32) | inode->l32Size));
    } else {
        args.size       = inode->l32Size;
    }

    vNode_initStruct(vnode, &args);
    
    return vnode;
    ERROR_FINAL_BEGIN(0);
    if (ext2vnode != NULL) {
        mm_free(ext2vnode);
    }
    
    return NULL;
}

static void __ext2_fscore_closeVnode(FScore* fscore, vNode* vnode) {
    if (REF_COUNTER_DEREFER(vnode->refCounter) != 0) {
        return;
    }

    EXT2vnode* ext2vnode = HOST_POINTER(vnode, EXT2vnode, vnode);
    EXT2fscore* ext2fscore = HOST_POINTER(fscore, EXT2fscore, fscore);

    Index32 inodeID = vnode->vnodeID;
    EXT2SuperBlock* superBlock = ext2fscore->superBlock;
    Index32 blockGroupID = ext2SuperBlock_inodeID2BlockGroupIndex(superBlock, inodeID);

    DEBUG_ASSERT_SILENT(blockGroupID < ext2fscore->blockGroupNum);

    EXT2blockGroupDescriptor* desc = &ext2fscore->blockGroupTables[blockGroupID];
    
    Index32 inodeIndexInBlockGroup = (inodeID - 1) % superBlock->blockGroupInodeNum;
    Index32 inodeBlockIndex = desc->inodeTableIndex + (inodeIndexInBlockGroup * sizeof(EXT2inode)) / EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superBlock->blockSizeShift);
    Index32 inodeBlockOffset = (inodeIndexInBlockGroup * sizeof(EXT2inode)) % EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superBlock->blockSizeShift);

    Size granularity = fscore->blockDevice->device.granularity;
    Size deviceBlockSize = POWER_2(granularity);
    Index32 inodeDeviceBlockIndex = ext2SuperBlock_blockIndexFS2device(superBlock, granularity, inodeBlockIndex, inodeBlockOffset);
    Index32 inodeDeviceBlockOffset = ext2SuperBlock_blockOffsetFS2device(superBlock, granularity, inodeBlockIndex, inodeBlockOffset);

    DEBUG_ASSERT_SILENT(inodeDeviceBlockOffset + sizeof(EXT2inode) <= deviceBlockSize);

    Uint8 deviceBlockBuffer[deviceBlockSize];
    blockDevice_readBlocks(fscore->blockDevice, inodeDeviceBlockIndex, deviceBlockBuffer, 1);
    ERROR_GOTO_IF_ERROR(0);
    
    memory_memcpy(deviceBlockBuffer + inodeDeviceBlockOffset, &ext2vnode->inode, sizeof(EXT2inode));

    blockDevice_writeBlocks(fscore->blockDevice, inodeDeviceBlockIndex, deviceBlockBuffer, 1);
    ERROR_GOTO_IF_ERROR(0);

    mm_free(ext2vnode);

    return;
    ERROR_FINAL_BEGIN(0);
}

static fsEntry* __ext2_fscore_openFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags) {
    fsEntry* ret = fscore_genericOpenFSentry(fscore, vnode, flags);
    ERROR_GOTO_IF_ERROR(0);

    ret->operations = &_ext2_fsEntryOperations;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}