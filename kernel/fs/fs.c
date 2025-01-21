#include<fs/fs.h>

#include<devices/block/blockDevice.h>
#include<devices/block/memoryBlockDevice.h>
#include<devices/terminal/terminalSwitch.h>
#include<fs/devfs/devfs.h>
#include<fs/fat32/fat32.h>
#include<fs/fsEntry.h>
#include<fs/fsutil.h>
#include<kernel.h>
#include<kit/util.h>
#include<memory/paging.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<error.h>

FS* rootFS = NULL, * devFS = NULL;
BlockDevice devfsBlockDevice;

typedef struct {
    OldResult  (*init)();
    OldResult  (*checkType)(BlockDevice* device);
    OldResult  (*open)(FS* fs, BlockDevice* device);
    OldResult  (*close)(FS* fs);
} __FileSystemSupport;

static __FileSystemSupport _supports[FS_TYPE_NUM] = {
    [FS_TYPE_FAT32] = {
        .init       = fat32_init,
        .checkType  = fat32_checkType,
        .open       = fat32_open,
        .close      = fat32_close
    },
    [FS_TYPE_DEVFS] = {
        .init       = devfs_init,
        .checkType  = devfs_checkType,
        .open       = devfs_open,
        .close      = devfs_close
    }
};

void fs_init() {
    if (firstBootablePartition == NULL) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    FStype type = fs_checkType(firstBootablePartition);
    if (type == FS_TYPE_UNKNOWN) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    if (_supports[type].init() != RESULT_SUCCESS) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    rootFS = memory_allocate(sizeof(FS));
    if (rootFS == NULL || fs_open(rootFS, firstBootablePartition) != RESULT_SUCCESS) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }
    
    void* region = paging_convertAddressP2V(memory_allocateFrame(DEVFS_BLOCKDEVICE_BLOCK_NUM * BLOCK_DEVICE_DEFAULT_BLOCK_SIZE / PAGE_SIZE));
    if (region == NULL) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    memoryBlockDevice_initStruct(&devfsBlockDevice, region, DEVFS_BLOCKDEVICE_BLOCK_NUM * BLOCK_DEVICE_DEFAULT_BLOCK_SIZE, "DEVFS_BLKDEVICE");
    ERROR_GOTO_IF_ERROR(0);

    if (_supports[FS_TYPE_DEVFS].init() != RESULT_SUCCESS) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    devFS = memory_allocate(sizeof(FS));
    if (devFS == NULL || fs_open(devFS, &devfsBlockDevice) != RESULT_SUCCESS) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    fsEntryIdentifier devMountIdentifier;
    if (fsEntryIdentifier_initStruct(&devMountIdentifier, "/dev", FS_ENTRY_TYPE_DIRECTORY) != RESULT_SUCCESS) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    if (superBlock_rawMount(rootFS->superBlock, &devMountIdentifier, devFS->superBlock, devFS->superBlock->rootDirDesc) != RESULT_SUCCESS) {
        ERROR_THROW(ERROR_ID_UNKNOWN, 0);  //TODO: Temporary solution
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

FStype fs_checkType(BlockDevice* device) {
    for (FStype i = 0; i < FS_TYPE_NUM; ++i) {
        if (_supports[i].checkType(device) == RESULT_SUCCESS) {
            return i;
        }
    }
    return FS_TYPE_NUM;
}

OldResult fs_open(FS* fs, BlockDevice* device) {
    FStype type = fs_checkType(device);
    return _supports[type].open(fs, device);
}

OldResult fs_close(FS* fs) {
    return _supports[fs->type].close(fs);
}

OldResult fs_fileRead(File* file, void* buffer, Size n) {
    return fsutil_fileRead(file, buffer, n);
}

OldResult fs_fileWrite(File* file, const void* buffer, Size n) {
    return fsutil_fileWrite(file, buffer, n);
}

Index64 fs_fileSeek(File* file, Int64 offset, Uint8 begin) {
    return fsutil_fileSeek(file, offset, begin);
}

OldResult fs_fileOpen(File* file, ConstCstring filepath, FCNTLopenFlags flags) {
    return fsutil_openfsEntry(rootFS->superBlock, filepath, file, flags);
}

OldResult fs_fileClose(File* file) {
    return fsutil_closefsEntry(file);
}

OldResult fs_fileStat(File* file, FS_fileStat* stat) {
    fsEntryDesc* desc = file->desc;
    iNode* iNode = file->iNode;
    SuperBlock* superBlock = iNode->superBlock;
    
    memory_memset(stat, 0, sizeof(FS_fileStat));
    stat->deviceID = iNode->device->id;
    stat->iNodeID = iNode->iNodeID;
    stat->nLink = 1;    //TODO: nLink not implemented actually
    Uint32 mode = desc->mode;
    switch (desc->type) {
    case FS_ENTRY_TYPE_FILE:
        FS_FILE_STAT_MODE_SET_TYPE(mode, FS_FILE_STAT_MODE_TYPE_REGULAR_FILE);
        break;
    case FS_ENTRY_TYPE_DIRECTORY:
        FS_FILE_STAT_MODE_SET_TYPE(mode, FS_FILE_STAT_MODE_TYPE_DIRECTORY);
        break;
    case FS_ENTRY_TYPE_DEVICE:
        FS_FILE_STAT_MODE_SET_TYPE(mode, FS_FILE_STAT_MODE_TYPE_BLOCK_DEVICE);
        break;
    default:
        FS_FILE_STAT_MODE_SET_TYPE(mode, 0);
        break;
    }
    stat->mode = mode;
    stat->uid = 0;  //TODO: User not implemented
    stat->gid = 0;
    if (desc->type == FS_ENTRY_TYPE_DEVICE) {
        stat->rDevice = iNode->device->id;
    }
    stat->size = desc->dataRange.length;
    stat->blockSize = POWER_2(superBlock->blockDevice->device.granularity);
    stat->blocks = iNode->sizeInBlock;
    stat->accessTime.second = desc->lastAccessTime;
    stat->modifyTime.second = desc->lastModifyTime;
    stat->createTime.second = desc->createTime;

    return RESULT_SUCCESS;
}