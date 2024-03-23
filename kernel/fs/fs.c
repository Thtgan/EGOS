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
#include<memory/kMalloc.h>
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
        .init       = FAT32_fs_init,
        .checkType  = FAT32_fs_checkType,
        .open       = FAT32_fs_open,
        .close      = FAT32_fs_close
    },
    [FS_TYPE_DEVFS] = {
        .init       = devfs_fs_init,
        .checkType  = devfs_fs_checkType,
        .open       = devfs_fs_open,
        .close      = devfs_fs_close
    }
};

Result fs_init() {
    if (firstBootablePartition == NULL) {
        ERROR_CODE_SET(ERROR_CODE_OBJECT_DEVICE, ERROR_CODE_STATUS_NOT_FOUND);
        return RESULT_FAIL;
    }

    FStype type = fs_checkType(firstBootablePartition);
    if (type == FS_TYPE_UNKNOWN) {
        ERROR_CODE_SET(ERROR_CODE_OBJECT_DEVICE, ERROR_CODE_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    if (_supports[type].init() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    rootFS = kMalloc(sizeof(FS));
    if (rootFS == NULL || fs_open(rootFS, firstBootablePartition) == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    
    void* region = convertAddressP2V(physicalPage_alloc(DEVFS_FS_BLOCKDEVICE_BLOCK_NUM * DEFAULT_BLOCK_SIZE / PAGE_SIZE, PHYSICAL_PAGE_ATTRIBUTE_PUBLIC));
    if (region == NULL || createMemoryBlockDevice(&devfsBlockDevice, region, DEVFS_FS_BLOCKDEVICE_BLOCK_NUM * DEFAULT_BLOCK_SIZE, "DEVFS_BLKDEVICE") == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (_supports[FS_TYPE_DEVFS].init() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    devFS = kMalloc(sizeof(FS));
    if (devFS == NULL || fs_open(devFS, &devfsBlockDevice) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    fsEntryIdentifier devMountIdentifier;
    if (fsEntryIdentifier_initStruct(&devMountIdentifier, "/dev", FS_ENTRY_TYPE_DIRECTORY) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (superBlock_rawMount(rootFS->superBlock, &devMountIdentifier, devFS->superBlock, devFS->superBlock->rootDirDesc) == RESULT_FAIL) {
        return RESULT_FAIL;
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
