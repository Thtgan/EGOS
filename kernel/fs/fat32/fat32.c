#include<fs/fat32/fat32.h>

#include<devices/block/blockDevice.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/fsEntry.h>
#include<fs/fat32/inode.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<structs/hashTable.h>
#include<system/pageTable.h>

Result FAT32_fs_init() {
    return RESULT_SUCCESS;
}

#define __FS_FAT32_BPB_SIGNATURE        0x29
#define __FS_FAT32_MINIMUM_CLUSTER_NUM  65525

Result FAT32_fs_checkType(BlockDevice* device) {
    void* BPBbuffer = allocateBuffer(device->bytePerBlockShift);
    if (BPBbuffer == NULL || blockDeviceReadBlocks(device, 0, BPBbuffer, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    FAT32BPB* BPB = (FAT32BPB*)BPBbuffer;
    Uint32 clusterNum = (device->availableBlockNum - BPB->reservedSectorNum - BPB->FATnum * BPB->sectorPerFAT) / BPB->sectorPerCluster - BPB->rootDirectoryClusterIndex;
    if (!(BPB->bytePerSector == POWER_2(device->bytePerBlockShift) && BPB->signature == __FS_FAT32_BPB_SIGNATURE && memcmp(BPB->systemIdentifier, "FAT32   ", 8) == 0 && clusterNum > __FS_FAT32_MINIMUM_CLUSTER_NUM)) {
        return RESULT_FAIL;
    }

    releaseBuffer(BPBbuffer, device->bytePerBlockShift);

    return RESULT_SUCCESS;
}

static Result __FAT32_fs_doOpen(FS* fs, BlockDevice* device, void* batchAllocated);

#define __FS_FAT32_BATCH_ALLOCATE_SIZE BATCH_ALLOCATE_SIZE((SuperBlock, 1), (FSentry, 1), (FSentryDesc, 1), (FAT32info, 1), (FAT32BPB, 1), (SinglyLinkedList, 16))

Result FAT32_fs_open(FS* fs, BlockDevice* device) {
    void* batchAllocated = kMalloc(__FS_FAT32_BATCH_ALLOCATE_SIZE);
    if (batchAllocated == NULL || __FAT32_fs_doOpen(fs, device, batchAllocated) == RESULT_FAIL) {
        if (batchAllocated != NULL) {
            kFree(batchAllocated);
        }

        return RESULT_FAIL;
    }
    
    return RESULT_SUCCESS;
}

static ConstCstring __FAT32_fs_name = "FAT32";
static SuperBlockOperations __FAT32_fs_superBlockOperations = {
    .openInode      = FAT32_iNode_open,
    .closeInode     = FAT32_iNode_close,
    .openFSentry    = FAT32_fsEntry_open,
    .closeFSentry   = fsEntry_genericClose
};

static Result __FAT32_fs_doOpen(FS* fs, BlockDevice* device, void* batchAllocated) {
    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (SuperBlock, superBlock, 1), (FSentry, rootDirectory, 1), (FSentryDesc, desc, 1), (FAT32info, info, 1), (FAT32BPB, BPB, 1), (SinglyLinkedList, iNodeHashChains, 16)
    );

    void* buffer = allocateBuffer(device->bytePerBlockShift);
    if (buffer == NULL || blockDeviceReadBlocks(device, 0, buffer, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    memcpy(BPB, buffer, sizeof(FAT32BPB));

    releaseBuffer(buffer, device->bytePerBlockShift);

    if (POWER_2(device->bytePerBlockShift) != BPB->bytePerSector) {
        return RESULT_FAIL;
    }

    info->FATrange                      = RANGE_N(BPB->FATnum, BPB->reservedSectorNum, BPB->sectorPerFAT);

    Uint32 rootDirectorrySectorBegin    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT, rootDirectorrySectorLength = DIVIDE_ROUND_UP(FS_ENTRY_FAT32_DIRECTORY_ENTRY_SIZE * BPB->rootDirectoryEntryNum, BPB->bytePerSector);

    Uint32 dataBegin                    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT - 2 * BPB->sectorPerCluster, dataLength = device->availableBlockNum - dataBegin;
    info->dataBlockRange                = RANGE(dataBegin, dataLength);

    Size clusterNum                     = DIVIDE_ROUND_DOWN(dataLength, BPB->sectorPerCluster);
    info->clusterNum                    = clusterNum;
    info->BPB                           = BPB;

    Size FATsizeInByte                  = BPB->sectorPerFAT << device->bytePerBlockShift;
    void* pFAT = pageAlloc(DIVIDE_ROUND_UP_SHIFT(FATsizeInByte, PAGE_SIZE_SHIFT), MEMORY_TYPE_NORMAL);
    if (pFAT == NULL) {
        return RESULT_FAIL;
    }

    Index32* FAT                        = convertAddressP2V(pFAT);
    if (blockDeviceReadBlocks(device, info->FATrange.begin, FAT, info->FATrange.length) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    info->FAT                           = FAT;

    Index32 firstFreeCluster = INVALID_INDEX, last = INVALID_INDEX;
    for (Index32 i = 0; i < clusterNum; ++i) {
        Index32 nextCluster = PTR_TO_VALUE(32, FAT + i);
        if (FAT32_cluster_getType(info, nextCluster) != FAT32_CLUSTER_TYPE_FREE) {
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

    FSentryDescInitStructArgs args = {
        .name       = NULL,
        .type       = FS_ENTRY_TYPE_DIRECTORY,
        .dataRange  = RANGE(
            (BPB->rootDirectoryClusterIndex * BPB->sectorPerCluster + info->dataBlockRange.begin) << device->bytePerBlockShift, 
            (FAT32_cluster_getChainLength(info, BPB->rootDirectoryClusterIndex) * BPB->sectorPerCluster) << device->bytePerBlockShift
            ),
        .parent     = NULL,
        .flags      = EMPTY_FLAGS,
    };

    if (FSentryDesc_initStruct(desc, &args) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    superBlock->device                  = device;
    superBlock->operations              = &__FAT32_fs_superBlockOperations;
    superBlock->rootDirectory           = NULL;
    superBlock->specificInfo            = info;
    hashTable_initStruct(&superBlock->openedInode, 16, iNodeHashChains, LAMBDA(Size, (HashTable* this, Object key) {
        return key % this->hashSize;
    }));

    superBlock->rootDirectory           = rootDirectory;

    fs->name                            = __FAT32_fs_name;
    fs->type                            = FS_TYPE_FAT32;
    fs->superBlock                      = superBlock;
    
    if (superBlock_rawOpenFSentry(superBlock, rootDirectory, desc) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result FAT32_fs_close(FS* fs) {
    SuperBlock* superBlock = fs->superBlock;
    FSentryDesc* desc = superBlock->rootDirectory->desc;
    if (superBlock_rawCloseFSentry(superBlock, superBlock->rootDirectory) == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    FSentryDesc_clearStruct(desc);
    FAT32info* info = fs->superBlock->specificInfo;

    for (int i = info->firstFreeCluster, next; i != FAT32_CLSUTER_END_OF_CHAIN; i = next) {
        next = PTR_TO_VALUE(32, info->FAT + i);
        PTR_TO_VALUE(32, info->FAT + i) = 0;
    }

    for (int i = 0; i < info->FATrange.n; ++i) {    //TODO: FAT is only saved here, maybe we can make it save at anytime
        if (blockDeviceWriteBlocks(superBlock->device, info->FATrange.begin + i * info->FATrange.length, info->FAT, info->FATrange.length) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }
    blockDeviceSynchronize(superBlock->device);

    Size FATsizeInByte = info->FATrange.length << fs->superBlock->device->bytePerBlockShift;
    memset(info->FAT, 0, DIVIDE_ROUND_UP_SHIFT(FATsizeInByte, PAGE_SIZE_SHIFT));
    pageFree(info->FAT);

    void* batchAllocated = fs;
    memset(batchAllocated, 0, __FS_FAT32_BATCH_ALLOCATE_SIZE);
    kFree(fs);

    return RESULT_FAIL;
}