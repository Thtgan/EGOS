#include<fs/fileSystem.h>

#include<devices/block/blockDevice.h>
#include<devices/terminal/terminalSwitch.h>
#include<error.h>
#include<fs/fsManager.h>
#include<fs/fat32/fat32.h>
#include<fs/fsSyscall.h>
#include<fs/phospherus/phospherus.h>

FileSystem* rootFileSystem = NULL;

typedef struct {
    Result (*initFileSystem)();
    bool (*checkFileSystem)(BlockDevice* device);
    Result (*deployFileSystem)(BlockDevice* device);
    FileSystem* (*openFileSystem)(BlockDevice* device);
    Result (*closeFileSystem)(FileSystem* fs);
} __FileSystemSupport;

static __FileSystemSupport _supports[FILE_SYSTEM_TYPE_NUM] = {
    [FILE_SYSTEM_TYPE_PHOSPHERUS] = {
        .initFileSystem     = phospherusInitFileSystem,
        .checkFileSystem    = phospherusCheckFileSystem,
        .deployFileSystem   = phospherusDeployFileSystem,
        .openFileSystem     = phospherusOpenFileSystem,
        .closeFileSystem    = phospherusCloseFileSystem
    },
    [FILE_SYSTEM_TYPE_FAT32] = {
        .initFileSystem     = FAT32initFileSystem,
        .checkFileSystem    = FAT32checkFileSystem,
        .deployFileSystem   = FAT32deployFileSystem,
        .openFileSystem     = FAT32openFileSystem,
        .closeFileSystem    = FAT32closeFileSystem
    }
};

static BlockDevice* __getBootFromDevice();

Result initFileSystem() {
    initFSManager();
    BlockDevice* blockDevice = __getBootFromDevice();
    if (blockDevice == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return RESULT_FAIL;
    }

    FileSystemType type = checkFileSystem(blockDevice);
    if (type == FILE_SYSTEM_TYPE_UNKNOWN) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    if (_supports[type].initFileSystem() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    rootFileSystem = openFileSystem(blockDevice);
    if (rootFileSystem == NULL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result deployFileSystem(BlockDevice* device, FileSystemType type) {
    if (type >= FILE_SYSTEM_TYPE_NUM) {
        return RESULT_FAIL;
    }

    return _supports[type].deployFileSystem(device);
}

FileSystemType checkFileSystem(BlockDevice* device) {
    for (FileSystemType i = 0; i < FILE_SYSTEM_TYPE_NUM; ++i) {
        if (_supports[i].checkFileSystem(device) == RESULT_SUCCESS) {
            return i;
        }
    }
    return FILE_SYSTEM_TYPE_NUM;
}

FileSystem* openFileSystem(BlockDevice* device) {
    FileSystem* ret = getDeviceFS(device->deviceID);
    if (ret != NULL) {
        return ret;
    }

    FileSystemType type = checkFileSystem(device);
    ret = _supports[type].openFileSystem(device);

    if (ret != NULL) {
        if (registerDeviceFS(ret) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return ret;
}

Result closeFileSystem(FileSystem* fs) {
    BlockDevice* device = fs->superBlock->device;
    if (unregisterDeviceFS(device->deviceID) == NULL) {
        return RESULT_FAIL;
    }

    return _supports[fs->type].closeFileSystem(fs);
}

static BlockDevice* __getBootFromDevice() {
    return getBlockDeviceByName("HDA-p0");  //TODO: Ugly, figure out a method to know which device we are booting from
}