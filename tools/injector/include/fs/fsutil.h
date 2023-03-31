#if !defined(__FS_UTIL_H)
#define __FS_UTIL_H

#include<fs/directory.h>
#include<fs/file.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<kit/types.h>

int tracePath(DirectoryEntry* entry, ConstCstring path, iNodeType type);

File* openFile(ConstCstring path);

int closeFile(File* file);

#define FSUTIL_SEEK_BEGIN   0
#define FSUTIL_SEEK_CURRENT 1
#define FSUTIL_SEEK_END     2

int seekFile(File* file, int64_t offset, uint8_t begin);

size_t readFile(File* file, void* buffer, size_t n);

size_t writeFile(File* file, const void* buffer, size_t n);

File* fileOpen(iNode* iNode);

int fileClose(File* file);

Directory* directoryOpen(iNode* iNode);

int directoryClose(Directory* directory);

Index64 iNodeCreate(ID deviceID, iNodeType type);

int iNodeDelete(ID iNodeID);

iNode* iNodeOpen(ID iNodeID);

int iNodeClose(iNode* iNode);

#endif // __FS_UTIL_H
