#include<fs/fileSystem.h>

#include<devices/block/blockDevice.h>
#include<devices/terminal/terminalSwitch.h>
#include<error.h>
#include<fs/fsManager.h>
#include<fs/fsSyscall.h>
#include<fs/phospherus/phospherus.h>

FileSystem* rootFileSystem = NULL;

static Result (*_initFuncs[FILE_SYSTEM_TYPE_NUM])() = {
    [FILE_SYSTEM_TYPE_PHOSPHERUS] = phospherusInitFileSystem
};

static Result (*_checkers[FILE_SYSTEM_TYPE_NUM])(BlockDevice*) = {
    [FILE_SYSTEM_TYPE_PHOSPHERUS] = phospherusCheckFileSystem
};

Result initFileSystem() {
    initFSManager();
    BlockDevice* hda = getBlockDeviceByName("hda");
    if (hda == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return RESULT_FAIL;
    }

    FileSystemType type = checkFileSystem(hda);
    if (type == FILE_SYSTEM_TYPE_UNKNOWN) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    if (_initFuncs[type]() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    rootFileSystem = openFileSystem(hda);
    if (rootFileSystem == NULL) {
        return RESULT_FAIL;
    }

    initFSsyscall();

    return RESULT_SUCCESS;
}

Result deployFileSystem(BlockDevice* device, FileSystemType type) {
    Result ret = RESULT_SUCCESS;
    switch (type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            ret = phospherusDeployFileSystem(device);
            break;
    }
    return ret;
}

FileSystemType checkFileSystem(BlockDevice* device) {
    for (FileSystemType i = 0; i < FILE_SYSTEM_TYPE_NUM; ++i) {
        if (_checkers[i](device) == RESULT_SUCCESS) {
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
        if (registerDeviceFS(ret) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return ret;
}

Result closeFileSystem(FileSystem* fs) {
    if (unregisterDeviceFS(fs->device) == NULL) {
        return RESULT_FAIL;
    }

    Result ret = RESULT_SUCCESS;
    switch (fs->type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            ret = phospherusCloseFileSystem(fs);
            break;
        default:
            SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
            ret = RESULT_FAIL;
    }

    return ret;
}