#include<fs/ext2/blockGroup.h>

#include<devices/blockDevice.h>
#include<fs/ext2/ext2.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/mm.h>
#include<structs/bitmap.h>
#include<system/pageTable.h>
#include<debug.h>

Index32 ext2blockGroupDescriptor_allocateBlock(EXT2blockGroupDescriptor* descriptor, EXT2fscore* fscore) {
    if (descriptor->freeBlcokNum == 0) {
        return INVALID_INDEX32;
    }

    EXT2SuperBlock* superblock = fscore->superBlock;
    
    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift), blockBitSize = blockSize * 8;
    BlockDevice* blockDevice = fscore->fscore.blockDevice;
    Size blockDeviceSize  = blockSize / POWER_2(blockDevice->device.granularity);

    void* bitmapBuffer = mm_allocatePages(DIVIDE_ROUND_UP(blockSize, PAGE_SIZE));
    if (bitmapBuffer == NULL) {
        return INVALID_INDEX32;
    }

    Bitmap bitmap;
    bitmap_initStruct(&bitmap, blockBitSize, bitmapBuffer);

    Size bitmapBlockSize = DIVIDE_ROUND_UP(superblock->blockGroupBlockNum, blockBitSize);

    Index32 ret = INVALID_INDEX32;
    for (int i = 0; i < bitmapBlockSize; ++i) {
        Index32 translatedIndex = (descriptor->blockUsageBitmapIndex + i) * blockDeviceSize;
        blockDevice_readBlocks(blockDevice, translatedIndex, bitmapBuffer, blockDeviceSize);

        if ((ret = bitmap_findFirstClear(&bitmap, 0)) != INVALID_INDEX32) {
            break;
        }
    }
    
    --descriptor->freeBlcokNum;

    mm_freePages(bitmapBuffer);

    return ret;
}

void ext2blockGroupDescriptor_freeBlock(EXT2blockGroupDescriptor* descriptor, EXT2fscore* fscore, Index32 index) {
    EXT2SuperBlock* superblock = fscore->superBlock;

    if (index >= superblock->blockGroupBlockNum) {
        return;
    }

    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift), blockBitSize = blockSize * 8;
    BlockDevice* blockDevice = fscore->fscore.blockDevice;
    Size blockDeviceSize  = blockSize / POWER_2(blockDevice->device.granularity);

    void* bitmapBuffer = mm_allocatePages(DIVIDE_ROUND_UP(blockSize, PAGE_SIZE));
    if (bitmapBuffer == NULL) {
        return;
    }

    Bitmap bitmap;
    bitmap_initStruct(&bitmap, blockBitSize, bitmapBuffer);

    Index32 translatedIndex = (descriptor->blockUsageBitmapIndex + index / blockBitSize) * blockDeviceSize;
    Index32 inBlockIndex = index % blockBitSize;
    blockDevice_readBlocks(blockDevice, translatedIndex, bitmapBuffer, blockDeviceSize);

    DEBUG_ASSERT_SILENT(bitmap_testBit(&bitmap, inBlockIndex));

    bitmap_clearBit(&bitmap, inBlockIndex);
    blockDevice_writeBlocks(blockDevice, translatedIndex, bitmapBuffer, blockDeviceSize);

    ++descriptor->freeBlcokNum;

    mm_freePages(bitmapBuffer);
}

Index32 ext2blockGroupDescriptor_allocateInode(EXT2blockGroupDescriptor* descriptor, EXT2fscore* fscore, bool isDirectory) {
    if (descriptor->freeInodeNum == 0) {
        return INVALID_INDEX32;
    }

    EXT2SuperBlock* superblock = fscore->superBlock;
    
    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift), blockBitSize = blockSize * 8;
    BlockDevice* blockDevice = fscore->fscore.blockDevice;
    Size blockDeviceSize  = blockSize / POWER_2(blockDevice->device.granularity);

    void* bitmapBuffer = mm_allocatePages(DIVIDE_ROUND_UP(blockSize, PAGE_SIZE));
    if (bitmapBuffer == NULL) {
        return INVALID_INDEX32;
    }

    Bitmap bitmap;
    bitmap_initStruct(&bitmap, blockBitSize, bitmapBuffer);

    Size bitmapBlockSize = DIVIDE_ROUND_UP(superblock->blockGroupBlockNum, blockBitSize);

    Index32 ret = INVALID_INDEX32;
    for (int i = 0; i < bitmapBlockSize; ++i) {
        Index32 translatedIndex = (descriptor->inodeUsageBitmapIndex + i) * blockDeviceSize;
        blockDevice_readBlocks(blockDevice, translatedIndex, bitmapBuffer, blockDeviceSize);

        if ((ret = bitmap_findFirstClear(&bitmap, 0)) != INVALID_INDEX32) {
            break;
        }
    }
    
    --descriptor->freeInodeNum;
    if (isDirectory) {
        ++descriptor->directoryNum;
    }

    mm_freePages(bitmapBuffer);

    return ret;
}

void ext2blockGroupDescriptor_freeInode(EXT2blockGroupDescriptor* descriptor, EXT2fscore* fscore, Index32 index) {
    EXT2SuperBlock* superblock = fscore->superBlock;

    if (index >= superblock->blockGroupBlockNum) {
        return;
    }

    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift), blockBitSize = blockSize * 8;
    BlockDevice* blockDevice = fscore->fscore.blockDevice;
    Size blockDeviceSize  = blockSize / POWER_2(blockDevice->device.granularity);

    void* bitmapBuffer = mm_allocatePages(DIVIDE_ROUND_UP(blockSize, PAGE_SIZE));
    if (bitmapBuffer == NULL) {
        return;
    }

    Bitmap bitmap;
    bitmap_initStruct(&bitmap, blockBitSize, bitmapBuffer);

    Index32 translatedIndex = (descriptor->inodeUsageBitmapIndex + index / blockBitSize) * blockDeviceSize;
    Index32 inBlockIndex = index % blockBitSize;
    blockDevice_readBlocks(blockDevice, translatedIndex, bitmapBuffer, blockDeviceSize);

    DEBUG_ASSERT_SILENT(bitmap_testBit(&bitmap, inBlockIndex));

    bitmap_clearBit(&bitmap, inBlockIndex);
    blockDevice_writeBlocks(blockDevice, translatedIndex, bitmapBuffer, blockDeviceSize);

    ++descriptor->freeBlcokNum;

    mm_freePages(bitmapBuffer);
}