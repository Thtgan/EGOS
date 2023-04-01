#if !defined(MEMORY_BLOCK_DEVICE_H)
#define MEMORY_BLOCK_DEVICE_H

#include<devices/block/blockDevice.h>
#include<kit/types.h>

/**
 * @brief Initialize a virtual block device works on memory, for now, only one memory device can be created at one time
 * 
 * @param size The size of device
 * 
 * @return Memory blovk device created, NULL if create failed
 */
BlockDevice* createMemoryBlockDevice(void* region, size_t size, ConstCstring name);

#endif // MEMORY_BLOCK_DEVICE_H
