#if !defined(__BLOCK_DEVICE_H)
#define __BLOCK_DEVICE_H

#include<kit/oop.h>
#include<kit/types.h>
#include<structs/hashTable.h>

#define BLOCK_SIZE 512

STRUCT_PRE_DEFINE(BlockDeviceOperation);

typedef struct {
    char name[32];
    ID deviceID;            //ID of the device
    Size availableBlockNum;

    Object handle;          //Something to assist the block device working, can be anything
    BlockDeviceOperation* operations;
    HashChainNode hashChainNode;
} BlockDevice;

STRUCT_PRIVATE_DEFINE(BlockDeviceOperation) {
    /**
     * @brief Read blocks from the block device, sets errorcode to indicate error
     * 
     * @param additionalData Additional data of the block device
     * @param blockIndex Index of the first block to read from
     * @param buffer Buffer to storage the read data
     * @param n Num of blocks to read
     * @return Result Result of the operation
     */
    Result (*readBlocks)(BlockDevice* device, Index64 blockIndex, void* buffer, Size n);

    /**
     * @brief Write blocks to the block device, sets errorcode to indicate error
     * 
     * @param additionalData Additional data of the block device
     * @param blockIndex Index of the first block to write to
     * @param buffer Buffer contains the data to write
     * @param n Num of blocks to write
     * @return Result Result of the operation
     */
    Result (*writeBlocks)(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n);
};

/**
 * @brief Initialize the block device
 * 
 * @return Result Result of the operation
 */
Result initBlockDevice();

/**
 * @brief Create a block device, with basic information set, other information lisk additional information must be set manually
 * 
 * @param name Name of the device, not allowed to be duplicated with registered device
 * @param availableBlockNum Number of block that device contains
 * @param operations Operation functions for block device
 * @return BlockDevice* Created block device, NULL if device has duplicated name with registered device
 */
BlockDevice* createBlockDevice(const char* name, Size availableBlockNum, BlockDeviceOperation* operations, Object additionalData);

/**
 * @brief Release created block device, be sure that this device is NOT REGISTERED
 * 
 * @param device Block device to delete
 */
void releaseBlockDevice(BlockDevice* device);

/**
 * @brief Register the block device, duplicated name not allowed, sets errorcode to indicate error
 * 
 * @param device Block device to register
 * @return Result Result of the operation
 */
Result registerBlockDevice(BlockDevice* device);

/**
 * @brief Unregister the block device
 * 
 * @param name Name of the block device to unregister
 * @return BlockDevice* If successed, return the unregistered device, otherwise NULL
 */
BlockDevice* unregisterBlockDevice(ID deviceID);

/**
 * @brief Search the block device by name
 * 
 * @param name Name of the block device
 * @return BlockDevice* Block device found, NULL if not exist
 */
BlockDevice* getBlockDeviceByName(const char* name);

/**
 * @brief Search the block device by ID
 * 
 * @param id ID of the block device
 * @return BlockDevice* Block device found, NULL if not exist
 */
BlockDevice* getBlockDeviceByID(ID id);

/**
 * @brief Packed function of block device operation, sets errorcode to indicate error
 * 
 * @param device Block device
 * @param blockIndex Index of block to read
 * @param buffer Buffer to write to
 * @param n Num of block(s) to read
 * @return Result Result of the operation
 */
static inline Result blockDeviceReadBlocks(BlockDevice* device, Index64 blockIndex, void* buffer, Size n) {
    return device->operations->readBlocks(device, blockIndex, buffer, n);
} 

/**
 * @brief Packed function of block device operation, sets errorcode to indicate error
 * 
 * @param device Block device
 * @param blockIndex Index of block to write
 * @param buffer Buffer to read from
 * @param n Num of block(s) to write
 * @return Result Result of the operation
 */
static inline Result blockDeviceWriteBlocks(BlockDevice* device, Index64 blockIndex, const void* buffer, Size n) {
    return device->operations->writeBlocks(device, blockIndex, buffer, n);
} 

#endif // __BLOCK_DEVICE_H
