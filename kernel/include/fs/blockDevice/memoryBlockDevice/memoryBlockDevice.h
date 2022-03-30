#if !defined(MEMORY_BLOCK_DEVICE_H)
#define MEMORY_BLOCK_DEVICE_H

#include<fs/blockDevice/blockDevice.h>
#include<stddef.h>

/**
 * @brief Initialize a virtual block device which works on memory
 * 
 * @param device Device to initialize
 * @param size The size of device
 */
void initMemoryBlockDevice(BlockDevice* device, size_t size);

#endif // MEMORY_BLOCK_DEVICE_H
