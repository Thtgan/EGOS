#include<fs/fileSystem.h>

#include<fs/fat32/fat32.h>
#include<mm/mm.h>

static Result (*_checkers[FILE_SYSTEM_TYPE_NUM])(Volume*) = {
    [FILE_SYSTEM_TYPE_FAT32] = FAT32checkFileSystem
};

static Result (*_openFuncs[FILE_SYSTEM_TYPE_NUM])(Volume*, FileSystem*) = {
    [FILE_SYSTEM_TYPE_FAT32] = FAT32openFileSystem
};

FileSystemType checkFileSystem(Volume* v) {
    for (FileSystemType i = 0; i < FILE_SYSTEM_TYPE_NUM; ++i) {
        if (_checkers[i](v) == RESULT_SUCCESS) {
            return i;
        }
    }
    return FILE_SYSTEM_TYPE_UNKNOWN;
}

Result openFileSystem(Volume* v) {
    if (v->fileSystem != NULL) {
        return RESULT_SUCCESS;
    }

    FileSystemType type = checkFileSystem(v);

    v->fileSystem = NULL;
    if (type != FILE_SYSTEM_TYPE_UNKNOWN) {
        FileSystem* fileSystem = bMalloc(sizeof(FileSystem));
        if (fileSystem == NULL) {
            return RESULT_ERROR;
        }

        if (_openFuncs[type](v, fileSystem) == RESULT_ERROR) {
            bFree(fileSystem, sizeof(FileSystem));
            return RESULT_ERROR;
        } else {
            v->fileSystem = fileSystem;
        }
    }

    return RESULT_SUCCESS;
}