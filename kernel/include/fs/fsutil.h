#if !defined(__FS_UTIL_H)
#define __FS_UTIL_H

#include<fs/directory.h>
#include<fs/fileSystem.h>
#include<kit/types.h>

int tracePath(DirectoryEntry* entry, ConstCstring path, iNodeType type);

size_t loadFile(ConstCstring path, void* buffer, Index64 begin, size_t n);

#endif // __FS_UTIL_H
