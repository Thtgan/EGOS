#if !defined(__BLOCK_DEVICE_H)
#define __BLOCK_DEVICE_H

#include<stddef.h>
#include<stdint.h>

#define BLOCK_SIZE 512

typedef uint64_t block_index_t; //-1 (0xFFFFFFFFFFFFFFFF) stands for NULL

typedef struct {
    char name[16];
    size_t availableBlockNum;

    void* additionalData;   //Something to assist the block device working, can be anything
    //          ^
    //          | Yes, pass the additional data manually, cause there is no "this" pointer in C
    //          +---------------------------------------+
    //                                                  |
    //                                                  |
    /**
     * @brief Read blocks from the block device
     * 
     * @param additionalData Additional data of the block device
     * @param blockIndex Index of the first block to read from
     * @param buffer Buffer to storage the read data
     * @param n Num of blocks to read
     */
    void (*readBlocks)(void* additionalData, block_index_t blockIndex, void* buffer, size_t n);

    /**
     * @brief Write blocks to the block device
     * 
     * @param additionalData Additional data of the block device
     * @param blockIndex Index of the first block to write to
     * @param buffer Buffer contains the data to write
     * @param n Num of blocks to write
     */
    void (*writeBlocks)(void* additionalData, block_index_t blockIndex, const void* buffer, size_t n);
} BlockDevice;

BlockDevice* createBlockDevice(const char* filePath, const char* deviceName, size_t base, size_t size);

void deleteBlockDevice(BlockDevice* device);

#endif // __BLOCK_DEVICE_H
