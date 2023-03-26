#include<fs/fileSystem.h>

#include<debug.h>
#include<devices/block/blockDevice.h>
#include<devices/terminal/terminalSwitch.h>
#include<fs/fsManager.h>
#include<fs/phospherus/phospherus.h>
#include<print.h>

FileSystem* rootFileSystem = NULL;

static void (*_initFuncs[FILE_SYSTEM_TYPE_NUM])() = {
    [FILE_SYSTEM_TYPE_PHOSPHERUS] = phospherusInitFileSystem
};

static bool (*_checkers[FILE_SYSTEM_TYPE_NUM])(BlockDevice*) = {
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

bool deployFileSystem(BlockDevice* device, FileSystemType type) {
    bool ret = false;
    switch (type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            ret = phospherusDeployFileSystem(device);
            break;
    }
    return ret;
}

FileSystemType checkFileSystem(BlockDevice* device) {
    for (FileSystemType i = 0; i < FILE_SYSTEM_TYPE_NUM; ++i) {
        if (_checkers[i](device)) {
            return i;
        }
    }
    return FILE_SYSTEM_TYPE_NUM;
}

FileSystem* openFileSystem(BlockDevice* device) {
    FileSystem* ret = getDeviceFS(device);
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
        registerDeviceFS(device, ret);
    }

    return ret;
}

bool closeFileSystem(BlockDevice* device) {
    FileSystem* fs = unregisterDeviceFS(device);
    if (fs == NULL) {
        return false;
    }

    switch (fs->type) {
        case FILE_SYSTEM_TYPE_PHOSPHERUS:
            phospherusCloseFileSystem(fs);
            break;
        default:
            printf(TERMINAL_LEVEL_DEBUG, "Closing unknown file system\n");
    }

    return true;
}

File* openFile(iNode* iNode) {
    return rootFileSystem->opearations->fileGlobalOperations->openFile(iNode);
}

int closeFile(File* file) {
    return rootFileSystem->opearations->fileGlobalOperations->closeFile(file);
}

Directory* openDirectory(iNode* iNode) {
    return rootFileSystem->opearations->directoryGlobalOperations->openDirectory(iNode);
}

int closeDirectory(Directory* directory) {
    return rootFileSystem->opearations->directoryGlobalOperations->closeDirectory(directory);
}

Index64 createInode(iNodeType type) {
    return rootFileSystem->opearations->iNodeGlobalOperations->createInode(rootFileSystem, type);
}

int deleteInode(Index64 iNodeBlock) {
    return rootFileSystem->opearations->iNodeGlobalOperations->deleteInode(rootFileSystem, iNodeBlock);
}

iNode* openInode(Index64 iNodeBlock) {
    return rootFileSystem->opearations->iNodeGlobalOperations->openInode(rootFileSystem, iNodeBlock);
}

int closeInode(iNode* iNode) {
    return rootFileSystem->opearations->iNodeGlobalOperations->closeInode(iNode);
}