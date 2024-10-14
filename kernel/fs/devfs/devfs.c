#include<fs/devfs/devfs.h>

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<fs/devfs/blockChain.h>
#include<fs/devfs/fsEntry.h>
#include<fs/devfs/inode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>

static ConstCstring __devfs_name = "DEVFS";
static SuperBlockOperations __devfs_superBlockOperations = {
    .openInode      = devfs_iNode_open,
    .closeInode     = devfs_iNode_close,
    .openfsEntry    = devfs_fsEntry_open,
    .closefsEntry   = fsEntry_genericClose,
    .create         = devfs_fsEntry_create,
    .flush          = devfs_superBlock_flush,
    .mount          = NULL,
    .unmount        = NULL
};

//TODO: Capsule these data
static bool _devfs_opened = false;

static DEVFSspecificInfo _devfs_specificInfo;

Result devfs_init() {
    return RESULT_SUCCESS;
}

Result devfs_checkType(BlockDevice* blockDevice) {
    return RESULT_SUCCESS;  //TODO: Not a good logic
}

#define __DEVFS_SUPERBLOCK_HASH_BUCKET  16
#define __DEVFS_BATCH_ALLOCATE_SIZE     BATCH_ALLOCATE_SIZE((SuperBlock, 1), (fsEntryDesc, 1), (SinglyLinkedList, __DEVFS_SUPERBLOCK_HASH_BUCKET), (SinglyLinkedList, __DEVFS_SUPERBLOCK_HASH_BUCKET), (SinglyLinkedList, __DEVFS_SUPERBLOCK_HASH_BUCKET))

Result devfs_open(FS* fs, BlockDevice* blockDevice) {
    Device* device = &blockDevice->device;
    if (_devfs_opened || device->capacity != DEVFS_BLOCKDEVICE_BLOCK_NUM) { //TODO: Make it flexible
        return RESULT_ERROR;
    }

    void* batchAllocated = memory_allocate(__DEVFS_BATCH_ALLOCATE_SIZE);    //TODO: Bad memory management
    if (batchAllocated == NULL) {
        return RESULT_ERROR;
    }

    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (SuperBlock, superBlock, 1),
        (fsEntryDesc, desc, 1),
        (SinglyLinkedList, openedInodeChains, __DEVFS_SUPERBLOCK_HASH_BUCKET),
        (SinglyLinkedList, mountedChains, __DEVFS_SUPERBLOCK_HASH_BUCKET),
        (SinglyLinkedList, fsEntryDescChains, __DEVFS_SUPERBLOCK_HASH_BUCKET)
    );

    devfs_blockChain_initStruct(&_devfs_specificInfo.chains);

    SuperBlockInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__devfs_superBlockOperations,
        .rootDirDesc        = desc,
        .specificInfo       = &_devfs_specificInfo,
        .openedInodeBucket  = __DEVFS_SUPERBLOCK_HASH_BUCKET,
        .openedInodeChains  = openedInodeChains,
        .mountedBucket      = __DEVFS_SUPERBLOCK_HASH_BUCKET,
        .mountedChains      = mountedChains,
        .fsEntryDescBucket  = __DEVFS_SUPERBLOCK_HASH_BUCKET,
        .fsEntryDescChains  = fsEntryDescChains
    };

    superBlock_initStruct(superBlock, &args);
    if (devfs_fsEntry_buildRootDir(superBlock) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    if (devfs_fsEntry_initRootDir(superBlock) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    fs->superBlock = superBlock;
    fs->name = __devfs_name;
    fs->type = FS_TYPE_DEVFS;

    _devfs_opened = true;
    
    return RESULT_SUCCESS;
}

Result devfs_close(FS* fs) {
    _devfs_opened = false;

    return RESULT_SUCCESS;
}

Result devfs_superBlock_flush(SuperBlock* superBlock) {
    return RESULT_SUCCESS;
}