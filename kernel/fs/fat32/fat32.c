#include<fs/fat32/fat32.h>

#include<devices/block/blockDevice.h>
#include<devices/device.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/fsEntry.h>
#include<fs/fat32/inode.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<structs/hashTable.h>
#include<system/pageTable.h>

OldResult fat32_init() {
    return RESULT_SUCCESS;
}

#define __FS_FAT32_BPB_SIGNATURE        0x29
#define __FS_FAT32_MINIMUM_CLUSTER_NUM  65525

OldResult fat32_checkType(BlockDevice* blockDevice) {
    Device* device = &blockDevice->device;
    void* BPBbuffer = memory_allocate(POWER_2(device->granularity));
    if (BPBbuffer == NULL || blockDevice_readBlocks(blockDevice, 0, BPBbuffer, 1) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    FAT32BPB* BPB = (FAT32BPB*)BPBbuffer;
    Uint32 clusterNum = (device->capacity - BPB->reservedSectorNum - BPB->FATnum * BPB->sectorPerFAT) / BPB->sectorPerCluster - BPB->rootDirectoryClusterIndex;
    if (!(BPB->bytePerSector == POWER_2(device->granularity) && BPB->signature == __FS_FAT32_BPB_SIGNATURE && memory_memcmp(BPB->systemIdentifier, "FAT32   ", 8) == 0 && clusterNum > __FS_FAT32_MINIMUM_CLUSTER_NUM)) {
        return RESULT_ERROR;
    }

    memory_free(BPBbuffer);

    return RESULT_SUCCESS;
}

static OldResult __fat32_doOpen(FS* fs, BlockDevice* device, void* batchAllocated);

#define __FS_FAT32_SUPERBLOCK_HASH_BUCKET   16
#define __FS_FAT32_BATCH_ALLOCATE_SIZE      BATCH_ALLOCATE_SIZE((SuperBlock, 1), (fsEntryDesc, 1), (FAT32info, 1), (FAT32BPB, 1), (SinglyLinkedList, __FS_FAT32_SUPERBLOCK_HASH_BUCKET), (SinglyLinkedList, __FS_FAT32_SUPERBLOCK_HASH_BUCKET), (SinglyLinkedList, __FS_FAT32_SUPERBLOCK_HASH_BUCKET))

OldResult fat32_open(FS* fs, BlockDevice* blockDevice) {
    void* batchAllocated = memory_allocate(__FS_FAT32_BATCH_ALLOCATE_SIZE);
    if (batchAllocated == NULL || __fat32_doOpen(fs, blockDevice, batchAllocated) != RESULT_SUCCESS) {
        if (batchAllocated != NULL) {
            memory_free(batchAllocated);
        }

        return RESULT_ERROR;
    }
    
    return RESULT_SUCCESS;
}

static ConstCstring __fat32_name = "FAT32";
static SuperBlockOperations __fat32_superBlockOperations = {
    .openInode      = fat32_iNode_open,
    .closeInode     = fat32_iNode_close,
    .openfsEntry    = fat32_fsEntry_open,
    .closefsEntry   = fsEntry_genericClose,
    .create         = fat32_fsEntry_create,
    .flush          = fat32_superBlock_flush,
    .mount          = superBlock_genericMount,
    .unmount        = superBlock_genericUnmount
};

static OldResult __fat32_doOpen(FS* fs, BlockDevice* blockDevice, void* batchAllocated) {
    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (SuperBlock, superBlock, 1),
        (fsEntryDesc, desc, 1),
        (FAT32info, info, 1),
        (FAT32BPB, BPB, 1),
        (SinglyLinkedList, openedInodeChains, __FS_FAT32_SUPERBLOCK_HASH_BUCKET),
        (SinglyLinkedList, mountedChains, __FS_FAT32_SUPERBLOCK_HASH_BUCKET),
        (SinglyLinkedList, fsEntryDescChains, __FS_FAT32_SUPERBLOCK_HASH_BUCKET)
    );

    Device* device = &blockDevice->device;
    void* buffer = memory_allocate(POWER_2(device->granularity));
    if (buffer == NULL || blockDevice_readBlocks(blockDevice, 0, buffer, 1) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    memory_memcpy(BPB, buffer, sizeof(FAT32BPB));

    memory_free(buffer);

    if (POWER_2(device->granularity) != BPB->bytePerSector) {
        return RESULT_ERROR;
    }

    info->FATrange                      = RANGE_N(BPB->FATnum, BPB->reservedSectorNum, BPB->sectorPerFAT);

    Uint32 rootDirectorrySectorBegin    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT, rootDirectorrySectorLength = DIVIDE_ROUND_UP(FAT32_FS_ENTRY_DIRECTORY_ENTRY_SIZE * BPB->rootDirectoryEntryNum, BPB->bytePerSector);

    Uint32 dataBegin                    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT - 2 * BPB->sectorPerCluster, dataLength = device->capacity - dataBegin;
    info->dataBlockRange                = RANGE(dataBegin, dataLength);

    Size clusterNum                     = DIVIDE_ROUND_DOWN(dataLength, BPB->sectorPerCluster);
    info->clusterNum                    = clusterNum;
    info->BPB                           = BPB;

    Size FATsizeInByte                  = BPB->sectorPerFAT * POWER_2(device->granularity);
    void* pFAT = memory_allocateFrame(DIVIDE_ROUND_UP_SHIFT(FATsizeInByte, PAGE_SIZE_SHIFT));
    if (pFAT == NULL) {
        return RESULT_ERROR;
    }

    Index32* FAT                        = paging_convertAddressP2V(pFAT);
    if (blockDevice_readBlocks(blockDevice, info->FATrange.begin, FAT, info->FATrange.length) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    info->FAT                           = FAT;

    Index32 firstFreeCluster = INVALID_INDEX, last = INVALID_INDEX;
    for (Index32 i = 0; i < clusterNum; ++i) {
        Index32 nextCluster = PTR_TO_VALUE(32, FAT + i);
        if (fat32_getClusterType(info, nextCluster) != FAT32_CLUSTER_TYPE_FREE) {
            continue;
        }

        if (firstFreeCluster == INVALID_INDEX) {
            firstFreeCluster = i;
        } else {
            PTR_TO_VALUE(32, FAT + last) = i;
        }

        last = i;
    }
    PTR_TO_VALUE(32, FAT + last) = FAT32_CLSUTER_END_OF_CHAIN;

    info->firstFreeCluster              = firstFreeCluster;

    fsEntryDescInitArgs args1 = {
        .name       = "",
        .parentPath = "",
        .type       = FS_ENTRY_TYPE_DIRECTORY,
        .dataRange  = RANGE(
            (BPB->rootDirectoryClusterIndex * BPB->sectorPerCluster + info->dataBlockRange.begin) * POWER_2(device->granularity), 
            (fat32_getClusterChainLength(info, BPB->rootDirectoryClusterIndex) * BPB->sectorPerCluster) * POWER_2(device->granularity)
            ),
        .flags      = EMPTY_FLAGS,
    };

    if (fsEntryDesc_initStruct(desc, &args1) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    SuperBlockInitArgs args2 = {
        .blockDevice        = blockDevice,
        .operations         = &__fat32_superBlockOperations,
        .rootDirDesc        = desc,
        .specificInfo       = info,
        .openedInodeBucket  = __FS_FAT32_SUPERBLOCK_HASH_BUCKET,
        .openedInodeChains  = openedInodeChains,
        .mountedBucket      = __FS_FAT32_SUPERBLOCK_HASH_BUCKET,
        .mountedChains      = mountedChains,
        .fsEntryDescBucket  = __FS_FAT32_SUPERBLOCK_HASH_BUCKET,
        .fsEntryDescChains  = fsEntryDescChains
    };

    superBlock_initStruct(superBlock, &args2);

    fs->name                            = __fat32_name;
    fs->type                            = FS_TYPE_FAT32;
    fs->superBlock                      = superBlock;

    return RESULT_SUCCESS;
}

OldResult fat32_close(FS* fs) {
    SuperBlock* superBlock = fs->superBlock;
    fsEntryDesc_clearStruct(superBlock->rootDirDesc);
    FAT32info* info = fs->superBlock->specificInfo;

    if (superBlock_rawFlush(superBlock) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Device* superBlockDevice = &superBlockBlockDevice->device;
    Size FATsizeInByte = info->FATrange.length * POWER_2(superBlockDevice->granularity);
    memory_memset(info->FAT, 0, FATsizeInByte);
    memory_freeFrame(paging_convertAddressV2P(info->FAT));

    void* batchAllocated = superBlock; //TODO: Ugly code
    memory_memset(batchAllocated, 0, __FS_FAT32_BATCH_ALLOCATE_SIZE);
    memory_free(batchAllocated);

    return RESULT_SUCCESS;
}

OldResult fat32_superBlock_flush(SuperBlock* superBlock) {
    FAT32info* info = superBlock->specificInfo;
    FAT32BPB* BPB = info->BPB;
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Device* superBlockDevice = &superBlockBlockDevice->device;

    Size FATsizeInByte = info->FATrange.length * POWER_2(superBlockDevice->granularity);
    Index32* FATcopy = memory_allocateFrame(DIVIDE_ROUND_UP_SHIFT(FATsizeInByte, PAGE_SIZE_SHIFT));
    if (FATcopy == NULL) {
        return RESULT_ERROR;
    }
    FATcopy = paging_convertAddressP2V(FATcopy);
    memory_memcpy(FATcopy, info->FAT, FATsizeInByte);

    for (int i = info->firstFreeCluster, next; i != FAT32_CLSUTER_END_OF_CHAIN; i = next) {
        next = PTR_TO_VALUE(32, FATcopy + i);
        PTR_TO_VALUE(32, FATcopy + i) = 0;
    }

    for (int i = 0; i < info->FATrange.n; ++i) {    //TODO: FAT is only saved here, maybe we can make it save at anytime
        if (blockDevice_writeBlocks(superBlockBlockDevice, info->FATrange.begin + i * info->FATrange.length, FATcopy, info->FATrange.length) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }
    }

    memory_freeFrame(paging_convertAddressV2P(FATcopy));

    if (blockDevice_flush(superBlockBlockDevice) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}