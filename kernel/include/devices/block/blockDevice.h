#if !defined(__BLOCK_DEVICE_H)
#define __BLOCK_DEVICE_H

#include<devices/block/blockDeviceTypes.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/hashTable.h>

#define BLOCK_SIZE 512

typedef uint64_t block_index_t; //-1 (0xFFFFFFFFFFFFFFFF) stands for NULL

STRUCT_PRE_DEFINE(BlockDeviceOperation);

RECURSIVE_REFER_STRUCT(BlockDevice) {
    char name[32];
    BlockDeviceType type;
    ID deviceID;            //ID of the device
    size_t availableBlockNum;

    Object additionalData;  //Something to assist the block device working, can be anything
    BlockDeviceOperation* operations;
    HashChainNode hashChainNode;
};

STRUCT_PRIVATE_DEFINE(BlockDeviceOperation) {
    /**
     * @brief Read blocks from the block device
     * 
     * @param additionalData Additional data of the block device
     * @param blockIndex Index of the first block to read from
     * @param buffer Buffer to storage the read data
     * @param n Num of blocks to read
     */
    int (*readBlocks)(BlockDevice* device, block_index_t blockIndex, void* buffer, size_t n);

    /**
     * @brief Write blocks to the block device
     * 
     * @param additionalData Additional data of the block device
     * @param blockIndex Index of the first block to write to
     * @param buffer Buffer contains the data to write
     * @param n Num of blocks to write
     */
    int (*writeBlocks)(BlockDevice* device, block_index_t blockIndex, const void* buffer, size_t n);
};

/**
 * @brief Initialize the block device manager
 */
void initBlockDeviceManager();

/**
 * @brief Create a block device, with basic information set, other information lisk additional information must be set manually
 * 
 * @param name Name of the device, not allowed to be duplicated with registered device
 * @param type Type of the device
 * @param availableBlockNum Number of block that device contains
 * @param operations Operation functions for block device
 * @return BlockDevice* Created block device, NULL if device has duplicated name with registered device
 */
BlockDevice* createBlockDevice(const char* name, BlockDeviceType type, size_t availableBlockNum, BlockDeviceOperation* operations, Object additionalData);

/**
 * @brief Delete created block device, be sure that this device is NOT REGISTERED
 * 
 * @param device Block device to delete
 */
void deleteBlockDevice(BlockDevice* device);

/**
 * @brief Register the block device, duplicated name not allowed
 * 
 * @param device Block device to register
 * @return BlockDevice* If register successed
 */
bool registerBlockDevice(BlockDevice* device);

/**
 * @brief Unregister the block device
 * 
 * @param name Name of the block device to unregister
 * @return BlockDevice* If successed, return the unregistered device, otherwise NULL
 */
BlockDevice* unregisterBlockDeviceByName(const char* name);

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

static inline int blockDeviceReadBlocks(BlockDevice* device, block_index_t blockIndex, void* buffer, size_t n) {
    return device->operations->readBlocks(device, blockIndex, buffer, n);
} 

static inline int blockDeviceWriteBlocks(BlockDevice* device, block_index_t blockIndex, const void* buffer, size_t n) {
    return device->operations->writeBlocks(device, blockIndex, buffer, n);
} 

#endif // __BLOCK_DEVICE_H
