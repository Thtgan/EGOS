#if !defined(__FS_DEVFS_DEVFS_H)
#define __FS_DEVFS_DEVFS_H

typedef struct Devfscore Devfscore;
typedef struct DevfsNodeMetadata DevfsNodeMetadata;

#include<devices/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsNode.h>
#include<fs/devfs/vnode.h>
#include<fs/fscore.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef struct Devfscore {
    FScore              fscore;
    DevfsDirectoryEntry rootDirEntry;
} Devfscore;

void devfs_init();

bool devfs_checkType(BlockDevice* blockDevice);

void devfs_open(FS* fs, BlockDevice* blockDevice);

void devfs_close(FS* fs);

Index64 devfscore_allocateMappingIndex();

void devfscore_releaseMappingIndex(Index64 mappingIndex);

DevfsDirectoryEntry* devfscore_getStorageMapping(Index64 mappingIndex);

void devfscore_setStorageMapping(Index64 mappingIndex, DevfsDirectoryEntry* mapTo);

#endif // __FS_DEVFS_DEVFS_H
