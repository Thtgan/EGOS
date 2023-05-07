#include<fs/fsutil.h>

#include<devices/block/blockDevice.h>
#include<devices/virtualDevice.h>
#include<error.h>
#include<fs/directory.h>
#include<fs/file.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<kernel.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<multitask/process.h>
#include<string.h>

/**
 * @brief Check is path legal, a legal path satisifies: no '\', '//', and not ends with '/'(Unless path itself is "/", path to root directory), path seperated by '/'
 * 
 * @param path Path to check
 * @return bool True if path is legal
 */
static bool __pathCheck(ConstCstring path);

static Result __doTracePath(DirectoryEntry* entry, ConstCstring path, iNodeType type, iNode** iNodePtr, Directory** directoryPtr);

static Result __doFileOpen(DirectoryEntry* entry, iNode** iNodePtr, File** filePtr, Index32* retPtr);

static Result __doCreateEntry(ConstCstring path, ConstCstring name, ID iNodeID, iNodeType type, iNode** iNodePtr, Directory** directoryPtr);

static Result __doDeleteEntry(ConstCstring path, iNodeType type, Cstring* copyPtr, iNode** iNodePtr, Directory** directoryPtr);

Result tracePath(DirectoryEntry* entry, ConstCstring path, iNodeType type) {
    iNode* iNode = NULL;
    Directory* directory = NULL;

    Result ret = __doTracePath(entry, path, type, &iNode, &directory);

    if (directory != NULL) {
        if (rawDirectoryClose(directory) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    if (iNode != NULL) {
        if (rawInodeClose(iNode) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return ret;
}

int fileOpen(ConstCstring path) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_FILE) == RESULT_FAIL && tracePath(&entry, path, INODE_TYPE_DEVICE) == RESULT_FAIL) {
        return -1;
    }

    iNode* iNode = NULL;
    File* file = NULL;
    Index32 ret = -1;

    Result res = __doFileOpen(&entry, &iNode, &file, &ret);

    if (res == RESULT_FAIL) {
        if (file != NULL) {
            if (rawFileClose(file) == RESULT_FAIL) {
                return -1;
            }
        }

        if (iNode != NULL) {
            if (rawInodeClose(iNode) == RESULT_FAIL) {
                return -1;
            }
        }

        return -1;
    }

    return ret;
}

Result fileClose(int file) {
    File* filePtr = releaseFileSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return RESULT_FAIL;
    }

    iNode* iNode = filePtr->iNode;
    if (rawFileClose(filePtr) == RESULT_FAIL || rawInodeClose(iNode) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result fileSeek(int file, int64_t offset, uint8_t begin) {
    File* filePtr = getFileFromSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return RESULT_FAIL;
    }

    Index64 base = filePtr->pointer;
    switch (begin)
    {
        case FSUTIL_SEEK_BEGIN:
            base = 0;
            break;
        case FSUTIL_SEEK_CURRENT:
            break;
        case FSUTIL_SEEK_END:
            base = filePtr->iNode->onDevice.dataSize;
            break;
        default:
            break;
    }
    base += offset;

    if ((int64_t)base < 0 || base > filePtr->iNode->onDevice.dataSize) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return RESULT_FAIL;
    }

    if (rawFileSeek(filePtr, base) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Index64 fileGetPointer(int file) {
    File* filePtr = getFileFromSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return INVALID_INDEX;
    }

    return filePtr->pointer;
}

Result fileRead(int file, void* buffer, size_t n) {
    File* filePtr = getFileFromSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return RESULT_FAIL;
    }

    Index64 before = filePtr->pointer;
    if (rawFileRead(filePtr, buffer, n) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result fileWrite(int file, const void* buffer, size_t n) {
    File* filePtr = getFileFromSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return RESULT_FAIL;
    }

    Index64 before = filePtr->pointer;
    if (rawFileWrite(filePtr, buffer, n) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result createEntry(ConstCstring path, ConstCstring name, ID iNodeID, iNodeType type) {
    iNode* iNode = NULL;
    Directory* directory = NULL;

    Result ret = __doCreateEntry(path, name, iNodeID, type, &iNode, &directory);

    if (directory != NULL) {
        if (rawDirectoryClose(directory) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    if (iNode != NULL) {
        if (rawInodeClose(iNode) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return ret;
}

Result deleteEntry(ConstCstring path, iNodeType type) {
    Cstring copy = NULL;
    iNode* iNode = NULL;
    Directory* directory = NULL;

    Result ret = __doDeleteEntry(path, type, &copy, &iNode, &directory);

    if (directory != NULL) {
        if (rawDirectoryClose(directory) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    if (iNode != NULL) {
        if (rawInodeClose(iNode) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    if (copy != NULL) {
        kFree(copy);
    }

    return ret;
}

File* rawFileOpen(iNode* iNode) {
    FileSystem* fs = openFileSystem(iNode->device);
    if (fs == NULL) {
        return NULL;
    }
    
    return fs->opearations->fileGlobalOperations->fileOpen(iNode);
}

Result rawFileClose(File* file) {
    FileSystem* fs = openFileSystem(file->iNode->device);
    if (fs == NULL) {
        return RESULT_FAIL;
    }

    return fs->opearations->fileGlobalOperations->fileClose(file);
}

Directory* rawDirectoryOpen(iNode* iNode) {
    FileSystem* fs = openFileSystem(iNode->device);
    if (fs == NULL) {
        return NULL;
    }

    return fs->opearations->directoryGlobalOperations->openDirectory(iNode);
}

Result rawDirectoryClose(Directory* directory) {
    FileSystem* fs = openFileSystem(directory->iNode->device);
    if (fs == NULL) {
        return RESULT_FAIL;
    }

    return fs->opearations->directoryGlobalOperations->closeDirectory(directory);
}

Index64 rawInodeCreate(ID deviceID, iNodeType type) {
    if (type == INODE_TYPE_DEVICE) {
        return INVALID_INDEX;
    }

    BlockDevice* device = getBlockDeviceByID(deviceID);
    if (device == NULL) {
        return INVALID_INDEX;
    }

    FileSystem* fs = openFileSystem(device);
    if (fs == NULL) {
        return INVALID_INDEX;
    }

    return fs->opearations->iNodeGlobalOperations->createInode(fs, type);
}

Result rawInodeDelete(ID iNodeID) {
    ID deviceID = INODE_ID_GET_DEVICE_ID(iNodeID);
    Index64 iNodeIndex = INODE_ID_GET_INODE_INDEX(iNodeID);

    BlockDevice* device = getBlockDeviceByID(deviceID);
    if (device == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return RESULT_FAIL;
    }

    FileSystem* fs = openFileSystem(device);
    if (fs == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return RESULT_FAIL;
    }

    return fs->opearations->iNodeGlobalOperations->deleteInode(fs, iNodeIndex);
}

iNode* rawInodeOpen(ID iNodeID) {
    ID deviceID = INODE_ID_GET_DEVICE_ID(iNodeID);
    Index64 iNodeIndex = INODE_ID_GET_INODE_INDEX(iNodeID);

    BlockDevice* device = getBlockDeviceByID(deviceID);
    if (device == NULL) {
        return NULL;
    }

    FileSystem* fs = openFileSystem(device);
    if (fs == NULL) {
        return NULL;
    }

    return fs->opearations->iNodeGlobalOperations->openInode(fs, iNodeIndex);
}

Result rawInodeClose(iNode* iNode) {
    FileSystem* fs = openFileSystem(iNode->device);
    if (fs == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return RESULT_FAIL;
    }

    return fs->opearations->iNodeGlobalOperations->closeInode(iNode);
}

static bool __pathCheck(ConstCstring path) {
    int len = strlen(path);
    if (len == 0 || strchr(path, '\\') != NULL || strstr(path, "//") != NULL || (len > 1 && path[len - 1] == '/')) {
        return false;
    }

    return true;
}

static Result __doTracePath(DirectoryEntry* entry, ConstCstring path, iNodeType type, iNode** iNodePtr, Directory** directoryPtr) {
    if (!__pathCheck(path)) {
        SET_ERROR_CODE(ERROR_OBJECT_ARGUMENT, ERROR_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    if (*path == '/') {
        ++path;
    }

    DirectoryEntry directoryEntry;
    memset(&directoryEntry, 0, sizeof(DirectoryEntry));
    directoryEntry.iNodeID = BUILD_INODE_ID(rootFileSystem->device, rootFileSystem->rootDirectoryInodeIndex);
    directoryEntry.type = INODE_TYPE_DIRECTORY;
    
    char tmp[64];
    while (*path != '\0') {
        iNode* iNode = NULL;
        *iNodePtr = iNode = rawInodeOpen(directoryEntry.iNodeID);
        if (iNode == NULL) {
            return RESULT_FAIL;
        }

        Directory* directory = NULL;
        *directoryPtr = directory = rawDirectoryOpen(iNode);
        if (directory == NULL) {
            return RESULT_FAIL;
        }

        int i = 0;
        for (; *path != '\0' && *path != '/'; ++i, ++path) {
            tmp[i] = *path;
        }
        tmp[i] = '\0';

        Index64 index = rawDirectoryLookupEntry(directory, tmp, *path == '\0' ? type : INODE_TYPE_DIRECTORY);
        if (index == INVALID_INDEX) {
            return RESULT_FAIL;
        }

        if (rawDirectoryReadEntry(directory, &directoryEntry, index) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (rawDirectoryClose(directory) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (rawInodeClose(iNode) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (*path == '/') {
            ++path;
        }
    }

    *iNodePtr = NULL, *directoryPtr = NULL;

    memcpy(entry, &directoryEntry, sizeof(DirectoryEntry));
    return RESULT_SUCCESS;
}

static Result __doFileOpen(DirectoryEntry* entry, iNode** iNodePtr, File** filePtr, Index32* retPtr) {
    iNode* iNode = NULL;
    *iNodePtr = iNode = rawInodeOpen(entry->iNodeID);
    if (iNode == NULL) {
        return RESULT_FAIL;
    }

    File* file = NULL;
    if (entry->type == INODE_TYPE_FILE) {
        *filePtr = file = rawFileOpen(iNode);
        if (file == NULL) {
            return RESULT_FAIL;
        }
    } else {
        *filePtr = file = kMalloc(sizeof(File), MEMORY_TYPE_NORMAL);
        if (file == NULL) {
            return RESULT_FAIL;
        }
    
        file->iNode = iNode;
        file->operations = ((VirtualDeviceINodeData*)iNode->onDevice.data)->fileOperations;
        file->pointer = 0;
    }

    if ((*retPtr = allocateFileSlot(getCurrentProcess(), file)) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __doCreateEntry(ConstCstring path, ConstCstring name, ID iNodeID, iNodeType type, iNode** iNodePtr, Directory** directoryPtr) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_DIRECTORY) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (type == INODE_TYPE_DEVICE) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_ACCESS_DENIED);
        return RESULT_FAIL;
    }

    iNode* iNode = NULL;
    *iNodePtr = iNode = rawInodeOpen(entry.iNodeID);
    if (iNode == NULL) {
        return RESULT_FAIL;
    }

    Directory* directory = NULL;
    *directoryPtr = directory = rawDirectoryOpen(iNode);
    if (directory == NULL) {
        return RESULT_FAIL;
    }

    Result ret = RESULT_SUCCESS;
    if (rawDirectoryLookupEntry(directory, name, type) != INVALID_INDEX) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
        return RESULT_FAIL;
    }

    if (rawDirectoryAddEntry(directory, iNodeID, INODE_TYPE_DIRECTORY, name) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __doDeleteEntry(ConstCstring path, iNodeType type, Cstring* copyPtr, iNode** iNodePtr, Directory** directoryPtr) {
    size_t len = strlen(path);
    Cstring copy = NULL;
    *copyPtr = copy = kMalloc(len + 1, MEMORY_TYPE_NORMAL);
    if (copy == NULL) {
        return RESULT_FAIL;
    }
    strcpy(copy, path);

    char* sep = strrchr(copy, '/');
    ConstCstring dir = NULL, entryName = NULL;
    *(sep + (sep == copy)) = '\0';
    dir = copy;
    entryName = path + (sep - copy) + 1;

    DirectoryEntry entry;
    if (tracePath(&entry, dir, INODE_TYPE_DIRECTORY) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    iNode* iNode = NULL;
    *iNodePtr = iNode = rawInodeOpen(entry.iNodeID);
    if (iNode == NULL) {
        return RESULT_FAIL;
    }

    Directory* directory = NULL;
    *directoryPtr = directory = rawDirectoryOpen(iNode);
    if (directory == NULL) {
        return RESULT_FAIL;
    }

    Index64 entryIndex = rawDirectoryLookupEntry(directory, entryName, type);
    if (entryIndex == INVALID_INDEX) {
        SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_NOT_FOUND);
        return RESULT_FAIL;
    }

    if (rawDirectoryRemoveEntry(directory, entryIndex) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}