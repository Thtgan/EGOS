#if !defined(__FS_UTIL_H)
#define __FS_UTIL_H

#include<fs/directory.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<kit/types.h>

int tracePath(FileSystem* fs, DirectoryEntry* entry, ConstCstring path, iNodeType type);

size_t loadFile(FileSystem* fs, ConstCstring path, void* buffer, Index64 begin, size_t n);

int execute(FileSystem* fs, ConstCstring path);

#endif // __FS_UTIL_H
