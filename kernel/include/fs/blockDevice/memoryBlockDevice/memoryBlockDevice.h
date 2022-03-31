#if !defined(MEMORY_BLOCK_DEVICE_H)
#define MEMORY_BLOCK_DEVICE_H

#include<fs/blockDevice/blockDevice.h>
#include<stddef.h>

/**
 * @brief Initialize a virtual block device works on memory
 * 
 * @param size The size of device
 * 
 * @return Memory blovk device created, NULL if create failed
 */
BlockDevice* createMemoryBlockDevice(size_t size);

/**
 * @brief Delete created memory block device
 * 
 * @param blockDevice Block device to delete, be sure it is unregistered
 */
void deleteMemoryBlockDevice(BlockDevice* blockDevice);

#endif // MEMORY_BLOCK_DEVICE_H
