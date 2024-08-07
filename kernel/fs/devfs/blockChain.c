#include<fs/devfs/blockChain.h>

#include<fs/fat32/fat32.h>
#include<kit/types.h>
#include<memory/memory.h>

void devfs_blockChain_initStruct(DevFSblockChains* blockChains) {
    for (int i = 0; i < DEVFS_BLOCKCHAIN_SIZE; ++i) {
        blockChains->nextBlocks[i] = i + 1;
    }
    blockChains->nextBlocks[DEVFS_BLOCKCHAIN_SIZE - 1] = INVALID_INDEX;
    blockChains->firstFreeBlock = 0;
}

void devfs_blockChain_clearStruct(DevFSblockChains* blockChains) {
    memory_memset(blockChains, 0, sizeof(DevFSblockChains));
}

Index8 devfs_blockChain_get(DevFSblockChains* blockChains, Index8 chainFirst, Index8 index) {
    Index8 ret = chainFirst;
    for (int i = 0; i < index; ++i) {
        ret = blockChains->nextBlocks[i];
        
        if (ret == INVALID_INDEX) {
            return INVALID_INDEX;
        }
    }

    return ret;
}

Size devfs_blockChain_getChainLength(DevFSblockChains* blockChains, Index8 chainFirst) {
    Size ret = 0;

    Index8 current = chainFirst;
    while (true) {
        if (current == INVALID_INDEX) {
            break;
        }

        current = blockChains->nextBlocks[current];
        ++ret;
    }

    return ret;
}

Index8 devfs_blockChain_allocChain(DevFSblockChains* blockChains, Size length) {
    Index8 current = blockChains->firstFreeBlock, last = INVALID_INDEX;
    for (int i = 0; i < length; ++i) {
        if (current == INVALID_INDEX) {
            return INVALID_INDEX;
        }

        last = current;
        current = blockChains->nextBlocks[current];
    }

    Index8 ret = blockChains->firstFreeBlock;
    blockChains->firstFreeBlock = current;
    blockChains->nextBlocks[last] = INVALID_INDEX;

    return ret;
}

void devfs_blockChain_freeChain(DevFSblockChains* blockChains, Index8 chainFirst) {
    Index8 current = chainFirst, last = INVALID_INDEX;
    while (current != INVALID_INDEX) {
        last = current;
        current = blockChains->nextBlocks[current];
    }

    blockChains->nextBlocks[last] = blockChains->firstFreeBlock;
    blockChains->firstFreeBlock = chainFirst;
}

Index8 devfs_blockChain_cutChain(DevFSblockChains* blockChains, Index8 block) {
    Index32 ret = blockChains->nextBlocks[block];
    blockChains->nextBlocks[block] = INVALID_INDEX;
    return ret;
}

void devfs_blockChain_insertChain(DevFSblockChains* blockChains, Index8 block, Index8 chainFirst) {
    Index32 current = chainFirst;
    while (blockChains->nextBlocks[current] != INVALID_INDEX) {
        current = blockChains->nextBlocks[current];
    }

    blockChains->nextBlocks[current] = blockChains->nextBlocks[block];
    blockChains->nextBlocks[block] = chainFirst;
}