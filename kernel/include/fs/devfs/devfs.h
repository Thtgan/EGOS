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

Result devfs_init();

Result devfs_checkType(BlockDevice* blockDevice);

Result devfs_open(FS* fs, BlockDevice* blockDevice);

Result devfs_close(FS* fs);

#endif // __FS_DEVFS_DEVFS_H
