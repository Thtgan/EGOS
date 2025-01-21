#if !defined(__DEVICES_BLOCK_BLOCKDEVICE_H)
#define __DEVICES_BLOCK_BLOCKDEVICE_H

typedef struct BlockDeviceInitArgs BlockDeviceInitArgs;
typedef struct BlockDevice BlockDevice;

#include<devices/device.h>
#include<devices/block/blockBuffer.h>
#include<kit/oop.h>
#include<kit/types.h>

#define BLOCK_DEVICE_DEFAULT_BLOCK_SIZE         512
#define BLOCK_DEVICE_DEFAULT_BLOCK_SIZE_SHIFT   9

typedef struct BlockDeviceInitArgs {
    DeviceInitArgs      deviceInitArgs;
} BlockDeviceInitArgs;

typedef struct BlockDevice {
    Device              device;
    BlockBuffer*        blockBuffer;
} BlockDevice;

void blockDevice_initStruct(BlockDevice* blockDevice, BlockDeviceInitArgs* args);

void blockDevice_readBlocks(BlockDevice* blockDevice, Index64 blockIndex, void* buffer, Size n);

void blockDevice_writeBlocks(BlockDevice* blockDevice, Index64 blockIndex, const void* buffer, Size n);

void blockDevice_flush(BlockDevice* blockDevice);

void blockDevice_probePartitions(BlockDevice* blockDevice);

#endif // __DEVICES_BLOCK_BLOCKDEVICE_H
