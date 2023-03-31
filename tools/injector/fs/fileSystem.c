#include<fs/fileSystem.h>

#include<blowup.h>
#include<devices/block/blockDevice.h>
#include<error.h>
#include<fs/fsManager.h>
#include<fs/phospherus/phospherus.h>
#include<stdio.h>

FileSystem* rootFileSystem = NULL;

static void (*_initFuncs[FILE_SYSTEM_TYPE_NUM])() = {
    [FILE_SYSTEM_TYPE_PHOSPHERUS] = phospherusInitFileSystem
};

static int (*_checkers[FILE_SYSTEM_TYPE_NUM])(BlockDevice*) = {
    [FILE_SYSTEM_TYPE_PHOSPHERUS] = phospherusCheckFileSystem
};

void initFileSystem() {
    initFSManager();
    BlockDevice* hda = getBlockDeviceByName("hda");
    if (hda == NULL) {
        blowup("Main device not found\n");
    }

    FileSystemType type = checkFileSystem(hda);
    if (type == FILE_SYSTEM_TYPE_UNKNOWN) {
        blowup("Unknown file system\n");
    }

    _initFuncs[type]();
    rootFileSystem = openFileSystem(hda);
}

int deployFileSystem(BlockDevice* device, FileSystemType type) {
    int ret = 0;
    switch (type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            ret = phospherusDeployFileSystem(device);
            break;
    }
    return ret;
}

FileSystemType checkFileSystem(BlockDevice* device) {
    for (FileSystemType i = 0; i < FILE_SYSTEM_TYPE_NUM; ++i) {
        if (_checkers[i](device) == 0) {
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

    switch (checkFileSystem(device)) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            ret = phospherusOpenFileSystem(device);
            break;
        default:
            ret = NULL;
    }

    if (ret != NULL) {
        registerDeviceFS(ret);
    }

    return ret;
}

int closeFileSystem(FileSystem* fs) {
    unregisterDeviceFS(fs->device);

    switch (fs->type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            phospherusCloseFileSystem(fs);
            break;
        default:
            printf("Closing unknown file system\n");
            SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
            return -1;
    }

    return 0;
}