#if !defined(__BLOCK_DEVICE_H)
#define __BLOCK_DEVICE_H

#include<stddef.h>
#include<stdint.h>
#include<structs/singlyLinkedList.h>

#define BLOCK_SIZE 512

typedef enum {
    BLOCK_DEVICE_TYPE_RAM,
    BLOCK_DEVICE_TYPE_DISK
} BlockDeviceType;

typedef struct {
    SinglyLinkedListNode node;

    char name[16];
    size_t availableBlockNum;
    BlockDeviceType type;

    void (*readBlock)(size_t block, void* buffer);
    void (*writeBlock)(size_t block, void* buffer);
} BlockDevice;

/**
 * @brief Initialize the block device manager
 */
void initBlockDeviceManager();

/**
 * @brief Register the block device, duplicated name not allowed
 * 
 * @param device Device to register
 * @return BlockDevice* If successed, return the registered device, otherwise NULL
 */
BlockDevice* registerBlockDevice(BlockDevice* device);

/**
 * @brief Search the block device By name
 * 
 * @param name Name of the block device
 * @return BlockDevice* Block device found, NULL if not exist
 */
BlockDevice* getBlockDeviceByName(const char* name);

/**
 * @brief Search the first block device match the required type, start at the next device of the begin
 * 
 * @param begin Beginning of the search, if NULL, search from the head of list
 * @param type Type to search
 * @return BlockDevice* Block device found, NULL if not exist
 */
BlockDevice* getBlockDeviceByType(BlockDevice* begin, BlockDeviceType type);

/**
 * @brief List the names of registered block devices, and the capacity of the device
 */
void printBlockDevices();

#endif // __BLOCK_DEVICE_H
