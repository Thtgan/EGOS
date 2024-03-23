#if !defined(__DEVFS_BLOCKCHAIN_H)
#define __DEVFS_BLOCKCHAIN_H

#include<devices/block/blockDevice.h>
#include<kit/types.h>

#define DEVFS_BLOCKCHAIN_SIZE       64

typedef struct {
    Uint8 nextBlocks[DEVFS_BLOCKCHAIN_SIZE];
    Uint8 firstFreeBlock;
} DevFSblockChains;

void DEVFS_blockChain_initStruct(DevFSblockChains* blockChains);

void DEVFS_blockChain_clearStruct(DevFSblockChains* blockChains);

Index8 DEVFS_blockChain_get(DevFSblockChains* blockChains, Index8 chainFirst, Index8 index);

Size DEVFS_blockChain_getChainLength(DevFSblockChains* blockChains, Index8 chainFirst);

Index8 DEVFS_blockChain_allocChain(DevFSblockChains* blockChains, Size length);

void DEVFS_blockChain_freeChain(DevFSblockChains* blockChains, Index8 chainFirst);

Index8 DEVFS_blockChain_cutChain(DevFSblockChains* blockChains, Index8 block);

void DEVFS_blockChain_insertChain(DevFSblockChains* blockChains, Index8 block, Index8 chainFirst);

#endif // __DEVFS_BLOCKCHAIN_H
