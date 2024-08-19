#if !defined(__DEVFS_H)
#define __DEVFS_H

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/devfs/blockChain.h>
#include<kit/types.h>

#define DEVFS_BLOCKDEVICE_BLOCK_NUM 64

typedef struct {
    DevFSblockChains chains;
} DEVFSspecificInfo;

Result devfs_init();

Result devfs_checkType(BlockDevice* blockDevice);

Result devfs_open(FS* fs, BlockDevice* blockDevice);

Result devfs_close(FS* fs);

#endif // __DEVFS_H
