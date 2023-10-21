#include<fs/fat32/fat32.h>

#include<devices/block/blockDevice.h>
#include<fs/fat32/fileSystemEntry.h>
#include<fs/fat32/inode.h>
#include<fs/fileSystem.h>
#include<fs/fileSystemEntry.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<structs/hashTable.h>
#include<system/pageTable.h>

Result FAT32initFileSystem() {
    return RESULT_SUCCESS;
}

Result FAT32deployFileSystem(BlockDevice* device) {
    //TODO: Not implemented yet
    return RESULT_FAIL;
}

#define __FAT32_BPB_SIGNATURE           0x29
#define __FAT32_MINIMUM_CLUSTER_NUM     65525

bool FAT32checkFileSystem(BlockDevice* device) {
    void* BPBbuffer = allocateBuffer(device->bytePerBlockShift);
    if (BPBbuffer == NULL || blockDeviceReadBlocks(device, 0, BPBbuffer, 1) == RESULT_FAIL) {
        return false;
    }

    FAT32BPB* BPB = (FAT32BPB*)BPBbuffer;
    Uint32 clusterNum = (device->availableBlockNum - BPB->reservedSectorNum - BPB->FATnum * BPB->sectorPerFAT) / BPB->sectorPerCluster - BPB->rootDirectoryClusterIndex;
    if (!(BPB->bytePerSector == POWER_2(device->bytePerBlockShift) && BPB->signature == __FAT32_BPB_SIGNATURE && memcmp(BPB->systemIdentifier, "FAT32   ", 8) == 0 && clusterNum > __FAT32_MINIMUM_CLUSTER_NUM)) {
        return false;
    }

    releaseBuffer(BPBbuffer, device->bytePerBlockShift);

    return true;
}

static Result __doFAT32openFileSystem(BlockDevice* device, void* batchAllocated);

FileSystem* FAT32openFileSystem(BlockDevice* device) {
    void* batchAllocated = kMalloc(BATCH_ALLOCATE_SIZE(FileSystem, SuperBlock, FileSystemEntry, FileSystemEntryDescriptor, FAT32info, FAT32BPB));
    FileSystem* ret = (FileSystem*)batchAllocated;

    if (batchAllocated == NULL || __doFAT32openFileSystem(device, batchAllocated) == RESULT_FAIL) {
        if (batchAllocated != NULL) {
            kFree(batchAllocated);
        }

        ret = NULL;
    }
    
    return ret;
}

static ConstCstring _name = "FAT32";
static SuperBlockOperations _FAT32superBlockOperations = {
    .openInode                  = FAT32openInode,
    .closeInode                 = FAT32closeInode,
    .openFileSystemEntry        = FAT32openFileSystemEntry,
    .closeFileSystemEntry       = genericCloseFileSystemEntry
};

static Result __doFAT32openFileSystem(BlockDevice* device, void* batchAllocated) {
    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, (FileSystem, fileSystem), (SuperBlock, superBlock), (FileSystemEntry, rootDirectory), (FileSystemEntryDescriptor, entryDescriptor), (FAT32info, info), (FAT32BPB, BPB));

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

    Uint32 rootDirectorrySectorBegin    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT, rootDirectorrySectorLength = DIVIDE_ROUND_UP(FAT32_DIRECTORY_ENTRY_SIZE * BPB->rootDirectoryEntryNum, BPB->bytePerSector);

    Uint32 dataBegin                    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT - 2 * BPB->sectorPerCluster, dataLength = device->availableBlockNum - dataBegin;
    info->dataBlockRange                = RANGE(dataBegin, dataLength);

    Size clusterNum                     = DIVIDE_ROUND_DOWN(dataLength, BPB->sectorPerCluster);
    info->clusterNum                    = clusterNum;
    info->BPB                           = BPB;

    Size FATsizeInByte                  = BPB->sectorPerFAT << device->bytePerBlockShift;
    Index32* FAT                        = convertAddressP2V(pageAlloc(DIVIDE_ROUND_UP_SHIFT(FATsizeInByte, PAGE_SIZE_SHIFT), MEMORY_TYPE_NORMAL));
    info->FAT                           = FAT;
    blockDeviceReadBlocks(device, info->FATrange.begin, info->FAT, info->FATrange.length);
    if (info->FAT == NULL || blockDeviceReadBlocks(device, info->FATrange.begin, info->FAT, info->FATrange.length) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Index32 firstFreeCluster = INVALID_INDEX, last = INVALID_INDEX;
    for (Index32 i = 0; i < clusterNum; ++i) {
        if (FAT32getClusterType(info, i) != FAT32_CLUSTER_TYPE_FREE) {
            continue;
        }

        if (firstFreeCluster == INVALID_INDEX) {
            firstFreeCluster = i;
        } else {
            PTR_TO_VALUE(32, FAT + last) = i;
        }

        last = i;
    }
    PTR_TO_VALUE(32, FAT + last) = FAT32_END_OF_CLUSTER_CHAIN;

    info->firstFreeCluster              = firstFreeCluster;

    FileSystemEntryDescriptorInitArgs args = {
        .name       = NULL,
        .type       = FILE_SYSTEM_ENTRY_TYPE_DIRECTOY,
        .dataRange  = RANGE(
            (BPB->rootDirectoryClusterIndex * BPB->sectorPerCluster + info->dataBlockRange.begin) << device->bytePerBlockShift, 
            (FAT32getClusterChainLength(info, BPB->rootDirectoryClusterIndex) * BPB->sectorPerCluster) << device->bytePerBlockShift
            ),
        .parent     = NULL,
        .flags      = EMPTY_FLAGS,
    };

    if (initFileSystemEntryDescriptor(entryDescriptor, &args) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    superBlock->device                  = device;
    superBlock->operations              = &_FAT32superBlockOperations;
    superBlock->rootDirectory           = NULL;
    superBlock->specificInfo            = info;

    if (rawSuperNodeOpenFileSystemEntry(superBlock, rootDirectory, entryDescriptor) == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    superBlock->rootDirectory           = rootDirectory;

    fileSystem->name                    = _name;
    fileSystem->type                    = FILE_SYSTEM_TYPE_FAT32;
    fileSystem->superBlock              = superBlock;
    initHashChainNode(&fileSystem->managerNode);

    return RESULT_SUCCESS;
}

Result FAT32closeFileSystem(FileSystem* fs) {
    SuperBlock* superBlock = fs->superBlock;
    FileSystemEntryDescriptor* descriptor = superBlock->rootDirectory->descriptor;
    if (rawSuperNodeCloseFileSystemEntry(superBlock, superBlock->rootDirectory) == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    clearFileSystemEntryDescriptor(descriptor);
    FAT32info* info = fs->superBlock->specificInfo;
    Size FATsizeInByte = info->FATrange.length << fs->superBlock->device->bytePerBlockShift;
    memset(info->FAT, 0, DIVIDE_ROUND_UP_SHIFT(FATsizeInByte, PAGE_SIZE_SHIFT));
    pageFree(info->FAT);

    void* batchAllocated = fs;
    memset(batchAllocated, 0, BATCH_ALLOCATE_SIZE(FileSystem, SuperBlock, FAT32info, FileSystemEntry));
    kFree(fs);

    return RESULT_FAIL;
}

FAT32ClusterType FAT32getClusterType(FAT32info* info, Index32 physicalClusterIndex) {
    if (physicalClusterIndex == 0x00000000) {
        return FAT32_CLUSTER_TYPE_FREE;
    }

    if (0x00000002 <= physicalClusterIndex && physicalClusterIndex < info->clusterNum) {
        return FAT32_CLUSTER_TYPE_ALLOCATERD;
    }

    if (info->clusterNum <= physicalClusterIndex && physicalClusterIndex < 0x0FFFFFF7) {
        return FAT32_CLUSTER_TYPE_RESERVED;
    }

    if (physicalClusterIndex == 0x0FFFFFF7) {
        return FAT32_CLUSTER_TYPE_BAD;
    }

    if (0x0FFFFFF8 <= physicalClusterIndex && physicalClusterIndex < 0x10000000) {
        return FAT32_CLUSTER_TYPE_EOF;
    }

    return FAT32_CLUSTER_TYPE_NOT_CLUSTER;
}

Index32 FAT32getCluster(FAT32info* info, Index32 firstCluster, Index32 index) {
    if (FAT32getClusterType(info, firstCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return INVALID_INDEX;
    }

    Index32* FAT = info->FAT;
    Index32 ret = firstCluster;
    for (int i = 0; i < index; ++i) {
        ret = PTR_TO_VALUE(32, FAT + ret);
    }
    return ret;
}

Size FAT32getClusterChainLength(FAT32info* info, Index32 firstCluster) {
    if (FAT32getClusterType(info, firstCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return -1;
    }

    Size ret = 1;
    Index32* FAT = info->FAT;

    Index32 current = firstCluster;
    while (true) {
        current = PTR_TO_VALUE(32, FAT + current);
        if (FAT32getClusterType(info, current) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            break;
        }
        ++ret;
    }

    return ret;
}

Index32 FAT32allocateClusterChain(FAT32info* info, Size length) {
    Index32* FAT = info->FAT;
    Index32 current = info->firstFreeCluster, last = INVALID_INDEX;
    for (int i = 0; i < length; ++i) {
        if (current == FAT32_END_OF_CLUSTER_CHAIN) {
            return INVALID_INDEX;
        }

        last = current;
        current = PTR_TO_VALUE(32, FAT + current);
    }

    Index32 ret = info->firstFreeCluster;
    info->firstFreeCluster = current;
    PTR_TO_VALUE(32, FAT + last) = FAT32_END_OF_CLUSTER_CHAIN;

    return ret;
}

void FAT32releaseClusterChain(FAT32info* info, Index32 clusterChainFirst) {
    Index32* FAT = info->FAT;
    Index32 current = info->firstFreeCluster, last = INVALID_INDEX;
    while (current != FAT32_END_OF_CLUSTER_CHAIN) {
        last = current;
        current = PTR_TO_VALUE(32, FAT + current);
    }

    PTR_TO_VALUE(32, FAT + last) = info->firstFreeCluster;
    info->firstFreeCluster = clusterChainFirst;
}

Index32 FAT32cutClusterChain(FAT32info* info, Index32 cluster) {
    if (FAT32getClusterType(info, cluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return -1;
    }

    Index32* FAT = info->FAT;
    Index32 ret = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = FAT32_END_OF_CLUSTER_CHAIN;
    return ret;
}

void FAT32insertClusterChain(FAT32info* info, Index32 cluster, Index32 clusterChainToInsert) {
    if (FAT32getClusterType(info, cluster) != FAT32_CLUSTER_TYPE_ALLOCATERD || FAT32getClusterType(info, clusterChainToInsert) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return;
    }

    Index32* FAT = info->FAT;
    Index32 current = clusterChainToInsert;
    while (PTR_TO_VALUE(32, FAT + current) != FAT32_END_OF_CLUSTER_CHAIN) {
        current = PTR_TO_VALUE(32, FAT + current);
    }

    PTR_TO_VALUE(32, FAT + current) = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = clusterChainToInsert;
}