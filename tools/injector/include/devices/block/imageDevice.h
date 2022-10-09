#if !defined(__IMAGE_DEVICE_H)
#define __IMAGE_DEVICE_H

#include<devices/block/blockDevice.h>
#include<kit/types.h>

BlockDevice* createImageDevice(const char* filePath, const char* deviceName, size_t base, size_t size);

void deleteImageDevice(BlockDevice* device);

#endif // __IMAGE_DEVICE_H
