#include<fs/devfs/devfs.h>

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsStructs.h>
#include<fs/devfs/blockChain.h>
#include<fs/devfs/fsEntry.h>
#include<fs/devfs/inode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>

static ConstCstring __devfs_fs_name = "DEVFS";
static SuperBlockOperations __devfs_fs_superBlockOperations = {
    .openInode      = devfs_iNode_open,
    .closeInode     = devfs_iNode_close,
    .openfsEntry    = devfs_fsEntry_open,
    .closefsEntry   = fsEntry_genericClose,
    .mount          = NULL,
    .unmount        = NULL
};

static bool _devfs_opened = false;

static DEVFSspecificInfo _devfsSpecificInfo;

Result devfs_fs_init() {
    return RESULT_SUCCESS;
}

Result devfs_fs_checkType(BlockDevice* device) {
    return RESULT_SUCCESS;
}

#define __DEVFS_SUPERBLOCK_HASH_BUCKET  16
#define __DEVFS_BATCH_ALLOCATE_SIZE     BATCH_ALLOCATE_SIZE((SuperBlock, 1), (fsEntryDesc, 1), (SinglyLinkedList, __DEVFS_SUPERBLOCK_HASH_BUCKET), (SinglyLinkedList, __DEVFS_SUPERBLOCK_HASH_BUCKET), (SinglyLinkedList, __DEVFS_SUPERBLOCK_HASH_BUCKET))

Result devfs_fs_open(FS* fs, BlockDevice* device) {
    if (_devfs_opened || device->availableBlockNum != DEVFS_FS_BLOCKDEVICE_BLOCK_NUM) { //TODO: Make it flexible
        return RESULT_FAIL;
    }

    void* batchAllocated = kMallocSpecific(__DEVFS_BATCH_ALLOCATE_SIZE, PHYSICAL_PAGE_ATTRIBUTE_PUBLIC, 16);    //TODO: Bad memory management
    if (batchAllocated == NULL) {
        return RESULT_FAIL;
    }

    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (SuperBlock, superBlock, 1),
        (fsEntryDesc, desc, 1),
        (SinglyLinkedList, openedInodeChains, __DEVFS_SUPERBLOCK_HASH_BUCKET),
        (SinglyLinkedList, mountedChains, __DEVFS_SUPERBLOCK_HASH_BUCKET),
        (SinglyLinkedList, fsEntryDescChains, __DEVFS_SUPERBLOCK_HASH_BUCKET)
    );

    DEVFS_blockChain_initStruct(&_devfsSpecificInfo.chains);

    SuperBlockInitArgs args = {
        .device             = device,
        .operations         = &__devfs_fs_superBlockOperations,
        .rootDirDesc        = desc,
        .specificInfo       = &_devfsSpecificInfo,
        .openedInodeBucket  = __DEVFS_SUPERBLOCK_HASH_BUCKET,
        .openedInodeChains  = openedInodeChains,
        .mountedBucket      = __DEVFS_SUPERBLOCK_HASH_BUCKET,
        .mountedChains      = mountedChains,
        .fsEntryDescBucket  = __DEVFS_SUPERBLOCK_HASH_BUCKET,
        .fsEntryDescChains  = fsEntryDescChains
    };

    superBlock_initStruct(superBlock, &args);
    if (devfs_fsEntry_buildRootDir(superBlock) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (devfs_fsEntry_initRootDir(superBlock) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    fs->superBlock = superBlock;
    fs->name = __devfs_fs_name;
    fs->type = FS_TYPE_DEVFS;

    _devfs_opened = true;
    
    return RESULT_SUCCESS;
}

Result devfs_fs_close(FS* fs) {
    _devfs_opened = false;

    return RESULT_SUCCESS;
}