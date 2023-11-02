#if !defined(MEMORY_BLOCK_DEVICE_H)
#define MEMORY_BLOCK_DEVICE_H

#include<devices/block/blockDevice.h>
#include<kit/types.h>

Result createMemoryBlockDevice(BlockDevice* device, void* region, Size size, ConstCstring name);

#endif // MEMORY_BLOCK_DEVICE_H
