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
        iNode* inode = iNodeOpen(directoryEntry.iNodeID);
        Directory* directory = directoryOpen(inode);
        int i = 0;
        for (; *path != '\0' && *path != '/'; ++i, ++path) {
            tmp[i] = *path;
        }
        tmp[i] = '\0';

        Index64 index = directoryLookupEntry(directory, tmp, *path == '\0' ? type : INODE_TYPE_DIRECTORY);
        if (index == INVALID_INDEX) {
            directoryClose(directory);
            iNodeClose(inode);
            SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_NOT_FOUND);
            return -1;
        }
        directoryReadEntry(directory, &directoryEntry, index);

        directoryClose(directory);
        iNodeClose(inode);

        if (*path == '/') {
            ++path;
        }
    }

    memcpy(entry, &directoryEntry, sizeof(DirectoryEntry));
    return 0;
}

File* openFile(ConstCstring path) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_FILE) == -1) {
        return NULL;
    }

    iNode* iNode = iNodeOpen(entry.iNodeID);
    File* file = fileOpen(iNode);

    return file;
}

File* openDeviceFile(ConstCstring path) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_DEVICE) == -1) {
        return NULL;
    }

    iNode* iNode = iNodeOpen(entry.iNodeID);
    if (iNode == NULL) {
        return NULL;
    }

    File* ret = kMalloc(sizeof(File), MEMORY_TYPE_NORMAL);
    
    ret->iNode = iNode;
    ret->operations = ((VirtualDeviceINodeData*)iNode->onDevice.data)->fileOperations;
    ret->pointer = 0;

    return ret;
}

int closeFile(File* file) {
    iNode* iNode = file->iNode;

    if (fileClose(file) == -1 || iNodeClose(iNode) == -1) {
        return -1;
    }

    return 0;
}

int seekFile(File* file, int64_t offset, uint8_t begin) {
    Index64 base = file->pointer;
    switch (begin)
    {
        case FSUTIL_SEEK_BEGIN:
            base = 0;
            break;
        case FSUTIL_SEEK_CURRENT:
            break;
        case FSUTIL_SEEK_END:
            base = file->iNode->onDevice.dataSize;
            break;
        default:
            break;
    }
    base += offset;

    if ((int64_t)base < 0 || base > file->iNode->onDevice.dataSize) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return -1;
    }

    fileSeek(file, base);

    return 0;
}

size_t readFile(File* file, void* buffer, size_t n) {
    Index64 before = file->pointer;
    if (fileRead(file, buffer, n) == -1) {
        return -1;
    }

    return file->pointer - before;
}

size_t writeFile(File* file, const void* buffer, size_t n) {
    Index64 before = file->pointer;
    if (fileWrite(file, buffer, n) == -1) {
        return -1;
    }

    return file->pointer - before;
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

    iNode* directoryInode = iNodeOpen(entry.iNodeID);
    Directory* directory = directoryOpen(directoryInode);

    int ret = 0;
    do {
        if (directoryLookupEntry(directory, name, type) != INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
            ret = -1;
            break;
        }

        if (directoryAddEntry(directory, iNodeID, INODE_TYPE_DIRECTORY, name) == -1) {
            iNodeDelete(iNodeID);
            ret = -1;
        }
    } while (0);

    directoryClose(directory);
    iNodeClose(directoryInode);

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

    iNode* directoryInode = iNodeOpen(entry.iNodeID);
    Directory* directory = directoryOpen(directoryInode);

    int ret = 0;
    Index64 entryIndex = directoryLookupEntry(directory, entryName, type);

    if (entryIndex == INVALID_INDEX) {
        SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_NOT_FOUND);
        ret = -1;
    } else {
        directoryRemoveEntry(directory, entryIndex);
    }

    directoryClose(directory);
    iNodeClose(directoryInode);
    kFree(copy);

    return ret;
}

File* fileOpen(iNode* iNode) {
    FileSystem* fs = openFileSystem(iNode->device);
    if (fs == NULL) {
        return NULL;
    }

    return fs->opearations->fileGlobalOperations->openFile(iNode);
}

int fileClose(File* file) {
    FileSystem* fs = openFileSystem(file->iNode->device);
    if (fs == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return -1;
    }

    return fs->opearations->fileGlobalOperations->closeFile(file);
}

Directory* directoryOpen(iNode* iNode) {
    FileSystem* fs = openFileSystem(iNode->device);
    if (fs == NULL) {
        return NULL;
    }

    return fs->opearations->directoryGlobalOperations->openDirectory(iNode);
}

int directoryClose(Directory* directory) {
    FileSystem* fs = openFileSystem(directory->iNode->device);
    if (fs == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return -1;
    }

    return fs->opearations->directoryGlobalOperations->closeDirectory(directory);
}

Index64 iNodeCreate(ID deviceID, iNodeType type) {
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

int iNodeDelete(ID iNodeID) {
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

iNode* iNodeOpen(ID iNodeID) {
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

int iNodeClose(iNode* iNode) {
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