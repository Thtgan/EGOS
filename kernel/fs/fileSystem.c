#include<fs/fileSystem.h>

#include<blowup.h>
#include<fs/phospherus/phospherus.h>

void initFileSystem(FileSystemTypes type) {
    switch (type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            phospherus_initFileSystem();
            break;
        default:
            return;
    }
}

bool deployFileSystem(BlockDevice* device, FileSystemTypes type) {
    bool ret = false;
    switch (type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            ret = phospherus_deployFileSystem(device);
            break;
    }
    return ret;
}

static bool (*_checkers[FILE_SYSTEM_TYPE_NULL])(BlockDevice*) = {
    [FILE_SYSTEM_TYPE_PHOSPHERUS] = phospherus_checkFileSystem
};

FileSystemTypes checkFileSystem(BlockDevice* device) {
    for (FileSystemTypes i = 0; i < FILE_SYSTEM_TYPE_NULL; ++i) {
        if (_checkers[i](device)) {
            return i;
        }
    }
    return FILE_SYSTEM_TYPE_NULL;
}

FileSystem* openFileSystem(BlockDevice* device, FileSystemTypes type) {
    FileSystem* ret = NULL;
    switch (type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            ret = phospherus_openFileSystem(device);
    }
    return ret;
}

void closeFileSystem(FileSystem* system) {
    switch (system->type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            phospherus_closeFileSystem(system);
            break;
        default:
            blowup("Closing unknown file system");
    }
}