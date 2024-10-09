#if !defined(__FS_DEVFS_BLOCKCHAIN_H)
#define __FS_DEVFS_BLOCKCHAIN_H

typedef struct DevFSblockChains DevFSblockChains;

#include<devices/block/blockDevice.h>
#include<kit/types.h>

#define DEVFS_BLOCKCHAIN_SIZE       64

typedef struct DevFSblockChains {
    Uint8 nextBlocks[DEVFS_BLOCKCHAIN_SIZE];
    Uint8 firstFreeBlock;
} DevFSblockChains;

void devfs_blockChain_initStruct(DevFSblockChains* blockChains);

void devfs_blockChain_clearStruct(DevFSblockChains* blockChains);

Index8 devfs_blockChain_get(DevFSblockChains* blockChains, Index8 chainFirst, Index8 index);

Size devfs_blockChain_getChainLength(DevFSblockChains* blockChains, Index8 chainFirst);

Index8 devfs_blockChain_allocChain(DevFSblockChains* blockChains, Size length);

void devfs_blockChain_freeChain(DevFSblockChains* blockChains, Index8 chainFirst);

Index8 devfs_blockChain_cutChain(DevFSblockChains* blockChains, Index8 block);

void devfs_blockChain_insertChain(DevFSblockChains* blockChains, Index8 block, Index8 chainFirst);

#endif // __FS_DEVFS_BLOCKCHAIN_H
