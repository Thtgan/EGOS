#if !defined(__DEVICES_MEMORYBLOCKDEVICE_H)
#define __DEVICES_MEMORYBLOCKDEVICE_H

#include<devices/blockDevice.h>
#include<kit/types.h>

typedef struct MemoryBlockDevice {
    BlockDevice blockDevice;
    void* regionBegin;
} MemoryBlockDevice;

void memoryBlockDevice_initStruct(MemoryBlockDevice* device, void* regionBegin, Size size, ConstCstring name);

#endif // __DEVICES_MEMORYBLOCKDEVICE_H
