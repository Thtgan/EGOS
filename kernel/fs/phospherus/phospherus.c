#include<fs/phospherus/phospherus.h>

#include<fs/fileSystem.h>
#include<fs/phospherus/allocator.h>
#include<fs/phospherus/phospherus_inode.h>
#include<fs/phospherus/phospherus_file.h>
#include<fs/phospherus/phospherus_directory.h>
#include<memory/kMalloc.h>
#include<returnValue.h>
#include<string.h>

FileSystemOperations fileSystemOperations;

void phospherusInitFileSystem() {
    phospherusInitAllocator();
    fileSystemOperations.iNodeGlobalOperations = phospherusInitInode();
    fileSystemOperations.fileGlobalOperations = phospherusInitFiles();
    fileSystemOperations.directoryGlobalOperations = phospherusInitDirectories();
}

ReturnValue phospherusDeployFileSystem(BlockDevice* device) {
    ReturnValue res;
    if (RETURN_VALUE_IS_ERROR(res = phospherusCheckBlockDevice(device))) {
        return res;
    }

    if (RETURN_VALUE_IS_ERROR(res = phospherusDeployAllocator(device))) {
        return res;
    }

    if (!RETURN_VALUE_IS_ERROR(res = phospherusCheckBlockDevice(device))) {
        return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_EXECUTION, RETURN_VALUE_STATUS_OPERATION_FAIL);
    }

    makeInode(device, RESERVED_BLOCK_ROOT_DIRECTORY, INODE_TYPE_DIRECTORY);

    return RETURN_VALUE_RETURN_NORMALLY;
}

ReturnValue phospherusCheckFileSystem(BlockDevice* device) {
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