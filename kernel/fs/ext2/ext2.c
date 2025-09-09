#include<fs/ext2/ext2.h>

#include<devices/blockDevice.h>
#include<devices/device.h>
#include<fs/ext2/vnode.h>
#include<kit/util.h>
#include<memory/mm.h>
#include<memory/memory.h>
#include<structs/refCounter.h>
#include<algorithms.h>
#include<error.h>

static vNode* __ext2_fscore_openVnode(FScore* fscore, fsNode* node);

#define __EXT2_SUPERBLOCK_OFFSET    1024

static ConstCstring __ext2_name = "EXT2";
static FScoreOperations __ext2_fscoreOperations = {
    .openVnode      = __ext2_fscore_openVnode,
    .closeVnode     = NULL,
    .sync           = NULL,
    .openFSentry    = NULL,
    .closeFSentry   = fscore_genericCloseFSentry,
    .mount          = fscore_genericMount,
    .unmount        = fscore_genericUnmount
};

void ext2_init() {
    
}

bool ext2_checkType(BlockDevice* blockDevice) {
    if (blockDevice == NULL) {
        return false;
    }

    Device* device = &blockDevice->device;
    Uint8 superBlockBuffer[algorithms_umax64(POWER_2(device->granularity), sizeof(EXT2SuperBlock))];

    blockDevice_readBlocks(blockDevice, __EXT2_SUPERBLOCK_OFFSET / POWER_2(device->granularity), (void*)superBlockBuffer, 1);   //TODO: One block?
    ERROR_GOTO_IF_ERROR(0);

    EXT2SuperBlock* superBlock = (EXT2SuperBlock*)(superBlockBuffer + (__EXT2_SUPERBLOCK_OFFSET % POWER_2(device->granularity)));
    bool ret = superBlock->signature == EXT2_SUPERBLOCK_IN_STORAGE_SIGNATURE && (EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superBlock->blockSizeShift) % POWER_2(device->granularity)) == 0;
    return ret;
    ERROR_FINAL_BEGIN(0);
    return false;
}

// #define __FS_EXT2_FSCORE_HASH_BUCKET    16
// #define __FS_EXT2_BATCH_ALLOCATE_SIZE   BATCH_ALLOCATE_SIZE((EXT2fscore, 1), (EXT2SuperBlock, 1), (SinglyLinkedList, __FS_EXT2_FSCORE_HASH_BUCKET))
#define __FS_EXT2_BATCH_ALLOCATE_SIZE   BATCH_ALLOCATE_SIZE((EXT2fscore, 1), (EXT2SuperBlock, 1))

void ext2_open(FS* fs, BlockDevice* blockDevice) {
    void* batchAllocated = NULL;
    Device* device = &blockDevice->device;
    Uint8 superBlockBuffer[algorithms_umax64(POWER_2(device->granularity), sizeof(EXT2SuperBlock))];

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

    blockDevice_readBlocks(blockDevice, __EXT2_SUPERBLOCK_OFFSET / POWER_2(device->granularity), (void*)superBlockBuffer, 1);   //TODO: One block?
    ERROR_GOTO_IF_ERROR(0);
    memory_memcpy(ext2SuperBlock, superBlockBuffer + (__EXT2_SUPERBLOCK_OFFSET % POWER_2(device->granularity)), sizeof(EXT2SuperBlock));

    FScoreInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__ext2_fscoreOperations,
        // .openedVnodeBucket  = __FS_EXT2_FSCORE_HASH_BUCKET,
        // .openedVnodeChains  = openedVnodeChains
    };

    fscore_initStruct(&ext2fscore->fscore, &args);
    ERROR_GOTO_IF_ERROR(0);
    
    fs->name    = __ext2_name;
    fs->type    = FS_TYPE_FAT32;
    fs->fscore  = &ext2fscore->fscore;
    
    return;
    ERROR_FINAL_BEGIN(0);
    
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

    ext2vnode = mm_allocate(sizeof(EXT2vnode));
    if (ext2vnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    return &ext2vnode->vnode;
    ERROR_FINAL_BEGIN(0);
    if (ext2vnode != NULL) {
        mm_free(ext2vnode);
    }
    
    return NULL;
}