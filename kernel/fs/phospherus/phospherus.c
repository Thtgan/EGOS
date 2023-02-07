#include<fs/phospherus/phospherus.h>

#include<fs/fileSystem.h>
#include<fs/phospherus/allocator.h>
#include<fs/phospherus/phospherus_inode.h>
#include<fs/phospherus/phospherus_file.h>
#include<fs/phospherus/phospherus_directory.h>
#include<memory/kMalloc.h>
#include<string.h>

FileSystemOperations fileSystemOperations;

void phospherusInitFileSystem() {
    phospherusInitAllocator();
    fileSystemOperations.iNodeGlobalOperations = phospherusInitInode();
    fileSystemOperations.fileGlobalOperations = phospherusInitFiles();
    fileSystemOperations.directoryGlobalOperations = phospherusInitDirectories();
}

bool phospherusDeployFileSystem(BlockDevice* device) {
    if (phospherusCheckBlockDevice(device)) {
        return false;
    }

    phospherusDeployAllocator(device);
    if (!phospherusCheckBlockDevice(device)) {
        return false;
    }

    makeInode(device, RESERVED_BLOCK_ROOT_DIRECTORY, INODE_TYPE_DIRECTORY);

    return true;
}

bool phospherusCheckFileSystem(BlockDevice* device) {
    return phospherusCheckBlockDevice(device);
}

FileSystem* phospherusOpenFileSystem(BlockDevice* device) {
    FileSystem* ret = kMalloc(sizeof(FileSystem), MEMORY_TYPE_NORMAL);

    strcpy(ret->name, "phospherus");
    ret->type = FILE_SYSTEM_TYPE_PHOSPHERUS;
    ret->opearations = &fileSystemOperations;
    ret->device = device->deviceID;
    ret->data = phospherusOpenAllocator(device);    //Open allocator to keep it in buffer, other operations can access it more quickly
    ret->rootDirectoryInode = RESERVED_BLOCK_ROOT_DIRECTORY;

    return ret;
}

void phospherusCloseFileSystem(FileSystem* fs) {
    phospherusCloseAllocator((PhospherusAllocator*)fs->data);
    kFree(fs);
}