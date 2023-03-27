#include<fs/fsutil.h>

#include<fs/directory.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<kernel.h>
#include<memory/memory.h>
#include<returnValue.h>
#include<string.h>

static bool __pathCheck(ConstCstring path);

static char _tmpStr[128];

ReturnValue tracePath(DirectoryEntry* entry, ConstCstring path, iNodeType type) {
    if (!__pathCheck(path)) {
        return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_ARGUMENT, RETURN_VALUE_STATUS_VERIFIVCATION_FAIL);
    }

    if (*path == '/') {
        ++path;
    }

    DirectoryEntry directoryEntry;
    directoryEntry.iNodeIndex = rootFileSystem->rootDirectoryInode;
    while (*path != '\0') {
        iNode* inode = openInode(directoryEntry.iNodeIndex);
        Directory* directory = openDirectory(inode);
        int i = 0;
        for (; *path != '\0' && *path != '/'; ++i, ++path) {
            _tmpStr[i] = *path;
        }
        _tmpStr[i] = '\0';

        Index64 index = directoryLookupEntry(directory, _tmpStr, *path == '\0' ? type : INODE_TYPE_DIRECTORY);
        if (index == -1) {
            closeDirectory(directory);
            closeInode(inode);
            return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_FILE, RETURN_VALUE_STATUS_NOT_FOUND);
        }
        directoryReadEntry(directory, &directoryEntry, index);

        closeDirectory(directory);
        closeInode(inode);

        if (*path == '/') {
            ++path;
        }
    }

    memcpy(entry, &directoryEntry, sizeof(DirectoryEntry));
    return RETURN_VALUE_RETURN_NORMALLY;
}

size_t loadFile(ConstCstring path, void* buffer, Index64 begin, size_t n) {
    DirectoryEntry entry;
    ReturnValue res = RETURN_VALUE_RETURN_NORMALLY;
    if (RETURN_VALUE_IS_ERROR(res = tracePath(&entry, path, INODE_TYPE_FILE))) {
        return res;
    }

    iNode* inode = openInode(entry.iNodeIndex);
    File* file = openFile(inode);

    fileSeek(file, begin);
    Index64 before = file->pointer, after = 0;
    fileRead(file, buffer, n == -1 ? inode->onDevice.dataSize : n);
    after = file->pointer == -1 ? inode->onDevice.dataSize : file->pointer;

    closeFile(file);
    closeInode(inode);

    return after - before;
}

static bool __pathCheck(ConstCstring path) {
    int len = strlen(path);
    if (len == 0 || strchr(path, '\\') != NULL || strstr(path, "//") != NULL || path[len - 1] == '/') {
        return false;
    }

    return true;
}