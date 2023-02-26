#include<fs/fsutil.h>

#include<fs/directory.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<memory/memory.h>
#include<string.h>

static bool __pathCheck(ConstCstring path);

static char _tmpStr[128];

int tracePath(FileSystem* fs, DirectoryEntry* entry, ConstCstring path, iNodeType type) {
    if (!__pathCheck(path)) {
        return -1;
    }

    if (*path == '/') {
        ++path;
    }

    DirectoryEntry directoryEntry;
    directoryEntry.iNodeIndex = fs->rootDirectoryInode;
    while (*path != '\0') {
        iNode* inode = fileSystemOpenInode(fs, directoryEntry.iNodeIndex);
        Directory* directory = fileSystemOpenDirectory(fs, inode);
        int i = 0;
        for (; *path != '\0' && *path != '/'; ++i, ++path) {
            _tmpStr[i] = *path;
        }
        _tmpStr[i] = '\0';

        Index64 index = directoryLookupEntry(directory, _tmpStr, *path == '\0' ? type : INODE_TYPE_DIRECTORY);
        if (index == -1) {
            fileSystemCloseDirectory(fs, directory);
            fileSystemCloseInode(fs, inode);
            return -1;
        }
        directoryReadEntry(directory, &directoryEntry, index);

        fileSystemCloseDirectory(fs, directory);
        fileSystemCloseInode(fs, inode);

        if (*path == '/') {
            ++path;
        }
    }

    memcpy(entry, &directoryEntry, sizeof(DirectoryEntry));
    return 0;
}

size_t loadFile(FileSystem* fs, ConstCstring path, void* buffer, Index64 begin, size_t n) {
    DirectoryEntry entry;
    if (tracePath(fs, &entry, path, INODE_TYPE_FILE) == -1) {
        return -1;
    }

    iNode* inode = fileSystemOpenInode(fs, entry.iNodeIndex);
    File* file = fileSystemOpenFile(fs, inode);

    fileSeek(file, begin);
    Index64 before = file->pointer, after = 0;
    fileRead(file, buffer, n == -1 ? inode->onDevice.dataSize : n);
    after = file->pointer == -1 ? inode->onDevice.dataSize : file->pointer;

    fileSystemCloseFile(fs, file);
    fileSystemCloseInode(fs, inode);

    return after - before;
}

static bool __pathCheck(ConstCstring path) {
    int len = strlen(path);
    if (len == 0 || strchr(path, '\\') != NULL || strstr(path, "//") != NULL || path[len - 1] == '/') {
        return false;
    }

    return true;
}