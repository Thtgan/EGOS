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

OldResult devfs_init();

OldResult devfs_checkType(BlockDevice* blockDevice);

OldResult devfs_open(FS* fs, BlockDevice* blockDevice);

OldResult devfs_close(FS* fs);

OldResult devfs_superBlock_flush(SuperBlock* superBlock);

#endif // __FS_DEVFS_DEVFS_H
