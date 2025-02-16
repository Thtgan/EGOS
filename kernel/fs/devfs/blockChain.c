#include<fs/devfs/blockChain.h>

#include<fs/fat32/fat32.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<error.h>

void devfs_blockChain_initStruct(DevfsBlockChains* blockChains) {
    for (int i = 0; i < DEVFS_BLOCK_CHAIN_CAPACITY; ++i) {
        blockChains->nextBlocks[i] = i + 1;
    }
    blockChains->nextBlocks[DEVFS_BLOCK_CHAIN_CAPACITY - 1] = INVALID_INDEX;
    blockChains->firstFreeBlock = 0;
}

void devfs_blockChain_clearStruct(DevfsBlockChains* blockChains) {
    memory_memset(blockChains, 0, sizeof(DevfsBlockChains));
}

Index8 devfs_blockChain_get(DevfsBlockChains* blockChains, Index8 firstBlock, Index8 index) {
    DEBUG_ASSERT_SILENT(firstBlock < DEVFS_BLOCK_CHAIN_CAPACITY);
    Index8 ret = firstBlock;
    for (int i = 0; i < index; ++i) {
        if (ret == (Index8)INVALID_INDEX) {
            ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
        }

        ret = blockChains->nextBlocks[ret];
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX;
}

Index8 devfs_blockChain_step(DevfsBlockChains* blockChains, Index8 firstBlock, Size n, Size* continousRet) {
    DEBUG_ASSERT_SILENT(firstBlock < DEVFS_BLOCK_CHAIN_CAPACITY);
    Size continousLength = 1;
    Index8 currentBlock = firstBlock;
    while (continousLength < n && blockChains->nextBlocks[currentBlock] == currentBlock + 1) {
        ++currentBlock;
        ++continousLength;
    }

    *continousRet = continousLength;

    return blockChains->nextBlocks[currentBlock];
}

Size devfs_blockChain_getChainLength(DevfsBlockChains* blockChains, Index8 firstBlock) {
    DEBUG_ASSERT_SILENT(firstBlock < DEVFS_BLOCK_CHAIN_CAPACITY);
    Size ret = 0;

    Index8 current = firstBlock;
    while (true) {
        if (current == (Index8)INVALID_INDEX) {
            break;
        }

        current = blockChains->nextBlocks[current];
        ++ret;
    }

    return ret;
}

Index8 devfs_blockChain_allocChain(DevfsBlockChains* blockChains, Size length) {
    Index8 current = blockChains->firstFreeBlock, last = INVALID_INDEX;
    for (int i = 0; i < length; ++i) {
        if (current == (Index8)INVALID_INDEX) {
            ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
        }

        last = current;
        current = blockChains->nextBlocks[current];
    }

    Index8 ret = blockChains->firstFreeBlock;
    blockChains->firstFreeBlock = current;
    blockChains->nextBlocks[last] = INVALID_INDEX;
    return ret;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX;
}

void devfs_blockChain_freeChain(DevfsBlockChains* blockChains, Index8 firstBlock) {
    DEBUG_ASSERT_SILENT(firstBlock < DEVFS_BLOCK_CHAIN_CAPACITY);
    Index8 current = firstBlock, last = INVALID_INDEX;
    while (current != (Index8)INVALID_INDEX) {
        last = current;
        current = blockChains->nextBlocks[current];
    }

    blockChains->nextBlocks[last] = blockChains->firstFreeBlock;
    blockChains->firstFreeBlock = firstBlock;
}

Index8 devfs_blockChain_cutChain(DevfsBlockChains* blockChains, Index8 block) {
    DEBUG_ASSERT_SILENT(block < DEVFS_BLOCK_CHAIN_CAPACITY);
    Index32 ret = blockChains->nextBlocks[block];
    blockChains->nextBlocks[block] = INVALID_INDEX;
    return ret;
}

void devfs_blockChain_insertChain(DevfsBlockChains* blockChains, Index8 block, Index8 firstBlock) {
    DEBUG_ASSERT_SILENT(block < DEVFS_BLOCK_CHAIN_CAPACITY);
    DEBUG_ASSERT_SILENT(firstBlock < DEVFS_BLOCK_CHAIN_CAPACITY);
    Index32 current = firstBlock;
    while (blockChains->nextBlocks[current] != INVALID_INDEX) {
        current = blockChains->nextBlocks[current];
    }

    blockChains->nextBlocks[current] = blockChains->nextBlocks[block];
    blockChains->nextBlocks[block] = firstBlock;
}