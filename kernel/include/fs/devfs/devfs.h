#if !defined(__FS_DEVFS_DEVFS_H)
#define __FS_DEVFS_DEVFS_H

typedef struct DEVFSspecificInfo DEVFSspecificInfo;

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/devfs/blockChain.h>
#include<kit/types.h>

#define DEVFS_BLOCKDEVICE_BLOCK_NUM 64

typedef struct DEVFSspecificInfo {
    DevFSblockChains chains;
} DEVFSspecificInfo;

void devfs_init();

bool devfs_checkType(BlockDevice* blockDevice);

void devfs_open(FS* fs, BlockDevice* blockDevice);

void devfs_close(FS* fs);

void devfs_superBlock_flush(SuperBlock* superBlock);

#endif // __FS_DEVFS_DEVFS_H
