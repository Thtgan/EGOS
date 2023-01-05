#include<fs/fileSystem.h>

#include<fs/phospherus/phospherus.h>
#include<print.h>

void initFileSystem(FileSystemType type) {
    switch (type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            phospherusInitFileSystem();
            break;
        default:
            return;
    }
}

bool deployFileSystem(BlockDevice* device, FileSystemType type) {
    bool ret = false;
    switch (type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            ret = phospherusDeployFileSystem(device);
            break;
    }
    return ret;
}

static bool (*_checkers[FILE_SYSTEM_TYPE_UNKNOWN])(BlockDevice*) = {
    [FILE_SYSTEM_TYPE_PHOSPHERUS] = phospherusCheckFileSystem
};

FileSystemType checkFileSystem(BlockDevice* device) {
    for (FileSystemType i = 0; i < FILE_SYSTEM_TYPE_UNKNOWN; ++i) {
        if (_checkers[i](device)) {
            return i;
        }
    }
    return FILE_SYSTEM_TYPE_UNKNOWN;
}

FileSystem* openFileSystem(BlockDevice* device, FileSystemType type) {
    FileSystem* ret = NULL;
    switch (type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            ret = phospherusOpenFileSystem(device);
            break;
        default:
            ret = NULL;
    }

    return ret;
}

void closeFileSystem(FileSystem* fs) {
    switch (fs->type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            phospherusCloseFileSystem(fs);
            break;
        default:
            printf("Closing unknown file system\n");
    }
}