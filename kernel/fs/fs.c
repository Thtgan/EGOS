#include<fs/fs.h>

#include<devices/block/blockDevice.h>
#include<devices/block/memoryBlockDevice.h>
#include<devices/terminal/terminalSwitch.h>
#include<error.h>
#include<fs/devfs/devfs.h>
#include<fs/fat32/fat32.h>
#include<fs/fsEntry.h>
#include<fs/fsSyscall.h>
#include<kernel.h>
#include<kit/util.h>
#include<memory/paging.h>
#include<memory/memory.h>
#include<structs/hashTable.h>

FS* rootFS = NULL, * devFS = NULL;
BlockDevice devfsBlockDevice;

typedef struct {
    Result  (*init)();
    Result  (*checkType)(BlockDevice* device);
    Result  (*open)(FS* fs, BlockDevice* device);
    Result  (*close)(FS* fs);
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

Result fs_init() {
    if (firstBootablePartition == NULL) {
        ERROR_CODE_SET(ERROR_CODE_OBJECT_DEVICE, ERROR_CODE_STATUS_NOT_FOUND);
        return RESULT_ERROR;
    }

    FStype type = fs_checkType(firstBootablePartition);
    if (type == FS_TYPE_UNKNOWN) {
        ERROR_CODE_SET(ERROR_CODE_OBJECT_DEVICE, ERROR_CODE_STATUS_VERIFIVCATION_FAIL);
        return RESULT_ERROR;
    }

    if (_supports[type].init() != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    rootFS = memory_allocate(sizeof(FS));
    if (rootFS == NULL || fs_open(rootFS, firstBootablePartition) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }
    
    void* region = paging_convertAddressP2V(memory_allocateFrame(DEVFS_BLOCKDEVICE_BLOCK_NUM * BLOCK_DEVICE_DEFAULT_BLOCK_SIZE / PAGE_SIZE));
    if (region == NULL || memoryBlockDevice_initStruct(&devfsBlockDevice, region, DEVFS_BLOCKDEVICE_BLOCK_NUM * BLOCK_DEVICE_DEFAULT_BLOCK_SIZE, "DEVFS_BLKDEVICE") != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    if (_supports[FS_TYPE_DEVFS].init() != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    devFS = memory_allocate(sizeof(FS));
    if (devFS == NULL || fs_open(devFS, &devfsBlockDevice) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    fsEntryIdentifier devMountIdentifier;
    if (fsEntryIdentifier_initStruct(&devMountIdentifier, "/dev", FS_ENTRY_TYPE_DIRECTORY) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    if (superBlock_rawMount(rootFS->superBlock, &devMountIdentifier, devFS->superBlock, devFS->superBlock->rootDirDesc) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}

FStype fs_checkType(BlockDevice* device) {
    for (FStype i = 0; i < FS_TYPE_NUM; ++i) {
        if (_supports[i].checkType(device) == RESULT_SUCCESS) {
            return i;
        }
    }
    return FS_TYPE_NUM;
}

Result fs_open(FS* fs, BlockDevice* device) {
    FStype type = fs_checkType(device);
    return _supports[type].open(fs, device);
}

Result fs_close(FS* fs) {
    return _supports[fs->type].close(fs);
}
