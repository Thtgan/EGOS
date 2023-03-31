#include<fs/phospherus/phospherus.h>

#include<error.h>
#include<fs/fileSystem.h>
#include<fs/phospherus/allocator.h>
#include<fs/phospherus/phospherus_inode.h>
#include<fs/phospherus/phospherus_file.h>
#include<fs/phospherus/phospherus_directory.h>
#include<malloc.h>
#include<string.h>

FileSystemOperations fileSystemOperations;

void phospherusInitFileSystem() {
    phospherusInitAllocator();
    fileSystemOperations.iNodeGlobalOperations = phospherusInitInode();
    fileSystemOperations.fileGlobalOperations = phospherusInitFiles();
    fileSystemOperations.directoryGlobalOperations = phospherusInitDirectories();
}

int phospherusDeployFileSystem(BlockDevice* device) {
    if (phospherusCheckBlockDevice(device) == 0) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
        return -1;
    }

    if (phospherusDeployAllocator(device) == -1) {
        return -1;
    }

    if (phospherusCheckBlockDevice(device) == -1) {
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return -1;
    }

    makeInode(device, RESERVED_BLOCK_ROOT_DIRECTORY, INODE_TYPE_DIRECTORY);

    return 0;
}

int phospherusCheckFileSystem(BlockDevice* device) {
    return phospherusCheckBlockDevice(device);
}

FileSystem* phospherusOpenFileSystem(BlockDevice* device) {
    FileSystem* ret = malloc(sizeof(FileSystem));

    strcpy(ret->name, "phospherus");
    ret->type = FILE_SYSTEM_TYPE_PHOSPHERUS;
    ret->opearations = &fileSystemOperations;
    ret->device = device->deviceID;
    ret->data = phospherusOpenAllocator(device);    //Open allocator to keep it in buffer, other operations can access it more quickly
    ret->rootDirectoryInodeIndex = RESERVED_BLOCK_ROOT_DIRECTORY;

    return ret;
}

void phospherusCloseFileSystem(FileSystem* fs) {
    phospherusCloseAllocator((PhospherusAllocator*)fs->data);
    free(fs);
}