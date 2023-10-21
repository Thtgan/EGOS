#include<fs/phospherus/phospherus.h>

#include<error.h>
#include<fs/fileSystem.h>
#include<fs/phospherus/allocator.h>
#include<fs/phospherus/phospherus_inode.h>
#include<fs/phospherus/phospherus_file.h>
#include<fs/phospherus/phospherus_directory.h>
#include<memory/kMalloc.h>
#include<string.h>

// FileSystemOperations fileSystemOperations;

Result phospherusInitFileSystem() {
    // phospherusInitAllocator();
    // fileSystemOperations.iNodeGlobalOperations = phospherusInitInode();
    // fileSystemOperations.fileGlobalOperations = phospherusInitFiles();
    // fileSystemOperations.directoryGlobalOperations = phospherusInitDirectories();

    // return RESULT_SUCCESS;

    return RESULT_FAIL;
}

Result phospherusDeployFileSystem(BlockDevice* device) {
    // if (phospherusCheckBlockDevice(device) == RESULT_SUCCESS) {
    //     SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
    //     return RESULT_FAIL;
    // }

    // if (phospherusDeployAllocator(device) == RESULT_FAIL) {
    //     return RESULT_FAIL;
    // }

    // if (phospherusCheckBlockDevice(device) == RESULT_FAIL) {
    //     return RESULT_FAIL;
    // }

    // if (makeInode(device, RESERVED_BLOCK_ROOT_DIRECTORY, INODE_TYPE_DIRECTORY) == RESULT_FAIL) {
    //     return RESULT_FAIL;
    // }

    // return RESULT_SUCCESS;
    return RESULT_FAIL;
}

bool phospherusCheckFileSystem(BlockDevice* device) {
    // return phospherusCheckBlockDevice(device);
    return RESULT_FAIL;
}

FileSystem* phospherusOpenFileSystem(BlockDevice* device) {
    // FileSystem* ret = kMalloc(sizeof(FileSystem));
    // if (ret == NULL) {
    //     return NULL;
    // }

    // strcpy(ret->name, "phospherus");
    // ret->type = FILE_SYSTEM_TYPE_PHOSPHERUS;
    // ret->opearations = &fileSystemOperations;
    // ret->device = device->deviceID;
    // ret->data = phospherusOpenAllocator(device);    //Open allocator to keep it in buffer, other operations can access it more quickly
    // if (ret->data == NULL) {
    //     kFree(ret);
    //     return NULL;
    // }
    // ret->rootDirectoryInodeIndex = RESERVED_BLOCK_ROOT_DIRECTORY;

    // return ret;
    return NULL;
}

Result phospherusCloseFileSystem(FileSystem* fs) {
    // if (phospherusCloseAllocator((PhospherusAllocator*)fs->data) == RESULT_FAIL) {
    //     return RESULT_FAIL;
    // }
    
    // kFree(fs);

    // return RESULT_SUCCESS;
    return RESULT_FAIL;
}