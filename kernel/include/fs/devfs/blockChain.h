#if !defined(__FS_DEVFS_BLOCKCHAIN_H)
#define __FS_DEVFS_BLOCKCHAIN_H

typedef struct DevfsBlockChains DevfsBlockChains;

#include<kit/types.h>

#define DEVFS_BLOCK_CHAIN_CAPACITY  64

typedef struct DevfsBlockChains {
    Uint8 nextBlocks[DEVFS_BLOCK_CHAIN_CAPACITY];
    Uint8 firstFreeBlock;
} DevfsBlockChains;

void devfs_blockChain_initStruct(DevfsBlockChains* blockChains);

void devfs_blockChain_clearStruct(DevfsBlockChains* blockChains);

Index8 devfs_blockChain_get(DevfsBlockChains* blockChains, Index8 firstBlock, Index8 index);

Index8 devfs_blockChain_step(DevfsBlockChains* blockChains, Index8 firstBlock, Size n, Size* continousRet);

Size devfs_blockChain_getChainLength(DevfsBlockChains* blockChains, Index8 firstBlock);

Index8 devfs_blockChain_allocChain(DevfsBlockChains* blockChains, Size length);

void devfs_blockChain_freeChain(DevfsBlockChains* blockChains, Index8 firstBlock);

Index8 devfs_blockChain_cutChain(DevfsBlockChains* blockChains, Index8 block);

void devfs_blockChain_insertChain(DevfsBlockChains* blockChains, Index8 block, Index8 firstBlock);

#endif // __FS_DEVFS_BLOCKCHAIN_H
