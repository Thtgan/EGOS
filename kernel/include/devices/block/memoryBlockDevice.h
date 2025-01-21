#if !defined(__DEVICES_BLOCK_MEMORYBLOCKDEVICE_H)
#define __DEVICES_BLOCK_MEMORYBLOCKDEVICE_H

#include<devices/block/blockDevice.h>
#include<kit/types.h>

void memoryBlockDevice_initStruct(BlockDevice* device, void* region, Size size, ConstCstring name);

#endif // __DEVICES_BLOCK_MEMORYBLOCKDEVICE_H
