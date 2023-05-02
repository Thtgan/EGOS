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

int tracePath(DirectoryEntry* entry, ConstCstring path, iNodeType type) {
    if (!__pathCheck(path)) {
        SET_ERROR_CODE(ERROR_OBJECT_ARGUMENT, ERROR_STATUS_VERIFIVCATION_FAIL);
        return -1;
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
        iNode* inode = rawInodeOpen(directoryEntry.iNodeID);
        Directory* directory = rawDirectoryOpen(inode);
        int i = 0;
        for (; *path != '\0' && *path != '/'; ++i, ++path) {
            tmp[i] = *path;
        }
        tmp[i] = '\0';

        Index64 index = rawDirectoryLookupEntry(directory, tmp, *path == '\0' ? type : INODE_TYPE_DIRECTORY);
        if (index == INVALID_INDEX) {
            rawDirectoryClose(directory);
            rawInodeClose(inode);
            SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_NOT_FOUND);
            return -1;
        }
        rawDirectoryReadEntry(directory, &directoryEntry, index);

        rawDirectoryClose(directory);
        rawInodeClose(inode);

        if (*path == '/') {
            ++path;
        }
    }

    memcpy(entry, &directoryEntry, sizeof(DirectoryEntry));
    return 0;
}

int fileOpen(ConstCstring path) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_FILE) == -1 && tracePath(&entry, path, INODE_TYPE_DEVICE) == -1) {
        return -1;
    }

    iNode* iNode = rawInodeOpen(entry.iNodeID);

    File* file = NULL;
    if (entry.type == INODE_TYPE_FILE) {
        file = rawFileOpen(iNode);
    } else {
        file = kMalloc(sizeof(File), MEMORY_TYPE_NORMAL);
    
        file->iNode = iNode;
        file->operations = ((VirtualDeviceINodeData*)iNode->onDevice.data)->fileOperations;
        file->pointer = 0;
    }

    Index32 ret = allocateFileSlot(getCurrentProcess(), file);

    return ret;
}

int fileClose(int file) {
    File* filePtr = releaseFileSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return -1;
    }

    iNode* iNode = filePtr->iNode;

    if (rawFileClose(filePtr) == -1 || rawInodeClose(iNode) == -1) {
        return -1;
    }

    return 0;
}

int fileSeek(int file, int64_t offset, uint8_t begin) {
    File* filePtr = getFileFromSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return -1;
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
        return -1;
    }

    rawFileSeek(filePtr, base);

    return 0;
}

Index64 fileGetPointer(int file) {
    File* filePtr = getFileFromSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return INVALID_INDEX;
    }

    return filePtr->pointer;
}

size_t fileRead(int file, void* buffer, size_t n) {
    File* filePtr = getFileFromSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return -1;
    }

    Index64 before = filePtr->pointer;
    if (rawFileRead(filePtr, buffer, n) == -1) {
        return -1;
    }

    return filePtr->pointer - before;
}

size_t fileWrite(int file, const void* buffer, size_t n) {
    File* filePtr = getFileFromSlot(getCurrentProcess(), file);
    if (filePtr == NULL) {
        return -1;
    }

    Index64 before = filePtr->pointer;
    if (rawFileWrite(filePtr, buffer, n) == -1) {
        return -1;
    }

    return filePtr->pointer - before;
}

int createEntry(ConstCstring path, ConstCstring name, ID iNodeID, iNodeType type) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    if (type == INODE_TYPE_DEVICE) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_ACCESS_DENIED);
        return -1;
    }

    iNode* directoryInode = rawInodeOpen(entry.iNodeID);
    Directory* directory = rawDirectoryOpen(directoryInode);

    int ret = 0;
    do {
        if (rawDirectoryLookupEntry(directory, name, type) != INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
            ret = -1;
            break;
        }

        if (rawDirectoryAddEntry(directory, iNodeID, INODE_TYPE_DIRECTORY, name) == -1) {
            rawInodeDelete(iNodeID);
            ret = -1;
        }
    } while (0);

    rawDirectoryClose(directory);
    rawInodeClose(directoryInode);

    return ret;
}

int deleteEntry(ConstCstring path, iNodeType type) {
    size_t len = strlen(path);
    Cstring copy = kMalloc(len + 1, MEMORY_TYPE_NORMAL);
    strcpy(copy, path);

    char* sep = strrchr(copy, '/');
    ConstCstring dir = NULL, entryName = NULL;
    *(sep + (sep == copy)) = '\0';
    dir = copy;
    entryName = path + (sep - copy) + 1;

    DirectoryEntry entry;
    if (tracePath(&entry, dir, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    iNode* directoryInode = rawInodeOpen(entry.iNodeID);
    Directory* directory = rawDirectoryOpen(directoryInode);

    int ret = 0;
    Index64 entryIndex = rawDirectoryLookupEntry(directory, entryName, type);

    if (entryIndex == INVALID_INDEX) {
        SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_NOT_FOUND);
        ret = -1;
    } else {
        rawDirectoryRemoveEntry(directory, entryIndex);
    }

    rawDirectoryClose(directory);
    rawInodeClose(directoryInode);
    kFree(copy);

    return ret;
}

File* rawFileOpen(iNode* iNode) {
    FileSystem* fs = openFileSystem(iNode->device);
    if (fs == NULL) {
        return NULL;
    }

    return fs->opearations->fileGlobalOperations->fileOpen(iNode);
}

int rawFileClose(File* file) {
    FileSystem* fs = openFileSystem(file->iNode->device);
    if (fs == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return -1;
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

int rawDirectoryClose(Directory* directory) {
    FileSystem* fs = openFileSystem(directory->iNode->device);
    if (fs == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return -1;
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

int rawInodeDelete(ID iNodeID) {
    ID deviceID = INODE_ID_GET_DEVICE_ID(iNodeID);
    Index64 iNodeIndex = INODE_ID_GET_INODE_INDEX(iNodeID);

    BlockDevice* device = getBlockDeviceByID(deviceID);
    if (device == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return -1;
    }

    FileSystem* fs = openFileSystem(device);
    if (fs == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return -1;
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

int rawInodeClose(iNode* iNode) {
    FileSystem* fs = openFileSystem(iNode->device);
    if (fs == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return -1;
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