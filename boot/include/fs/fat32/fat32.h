#if !defined(__FAT32_H)
#define __FAT32_H

#include<fs/fileSystem.h>
#include<fs/volume.h>

typedef struct {
    Volume* volume;
} FAT32context;

OldResult FAT32checkFileSystem(Volume* v);

OldResult FAT32openFileSystem(Volume* v, FileSystem* fileSystem);

OldResult test(Volume* v, Index32 cluster, Index32 offsetInCluster, FileSystemEntry* entry, Size* entrySizeInDevice);

#endif // __FAT32_H
