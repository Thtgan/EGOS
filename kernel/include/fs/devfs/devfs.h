#if !defined(__DEVFS_H)
#define __DEVFS_H

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/devfs/blockChain.h>
#include<kit/types.h>

#define DEVFS_FS_BLOCKDEVICE_BLOCK_NUM  64

typedef struct {
    DevFSblockChains chains;
} DEVFSspecificInfo;

Result devfs_fs_init();

Result devfs_fs_checkType(BlockDevice* device);

Result devfs_fs_open(FS* fs, BlockDevice* device);

Result devfs_fs_close(FS* fs);

#endif // __DEVFS_H
