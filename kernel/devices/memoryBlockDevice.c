#include<devices/memoryBlockDevice.h>

#include<devices/blockDevice.h>
#include<devices/device.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<cstring.h>
#include<structs/singlyLinkedList.h>
#include<error.h>

static void __memoryBlockDevice_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN);

static void __memoryBlockDevice_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN);

static void __memoryBlockDevice_flush(Device* device);

static DeviceOperations _memoryBlockDevice_deviceOperations = (DeviceOperations) {
    .readUnits  = __memoryBlockDevice_readUnits,
    .writeUnits = __memoryBlockDevice_writeUnits,
    .flush      = __memoryBlockDevice_flush
};

void memoryBlockDevice_initStruct(MemoryBlockDevice* device, void* regionBegin, Size size, ConstCstring name) {
    if (regionBegin == NULL) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    MajorDeviceID major = device_allocMajor();
    if (major == DEVICE_INVALID_ID) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    MinorDeviceID minor = device_allocMinor(major);
    if (minor == DEVICE_INVALID_ID) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size blockNum = DIVIDE_ROUND_DOWN(size, BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);
    memory_memset(regionBegin, 0, blockNum * BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);

    BlockDeviceInitArgs args = {
        .deviceInitArgs     = (DeviceInitArgs) {
            .id             = DEVICE_BUILD_ID(major, minor),
            .name           = name,
            .parent         = NULL,
            .granularity    = BLOCK_DEVICE_DEFAULT_BLOCK_SIZE_SHIFT,
            .capacity       = blockNum,
            .flags          = EMPTY_FLAGS,
            .operations     = &_memoryBlockDevice_deviceOperations
        },
    };

    blockDevice_initStruct(&device->blockDevice, &args);
    device->regionBegin = regionBegin;

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryBlockDevice_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN) {
    MemoryBlockDevice* memoryBlockDevice = HOST_POINTER(device, MemoryBlockDevice, blockDevice.device);
    Size blockSize = POWER_2(device->granularity);
    memory_memcpy(buffer, memoryBlockDevice->regionBegin + unitIndex * blockSize, unitN * blockSize);  //Just simple memcpy
}

static void __memoryBlockDevice_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN) {
    MemoryBlockDevice* memoryBlockDevice = HOST_POINTER(device, MemoryBlockDevice, blockDevice.device);
    Size blockSize = POWER_2(device->granularity);
    memory_memcpy(memoryBlockDevice->regionBegin + unitIndex * blockSize, buffer, unitN * blockSize);  //Just simple memcpy
}

static void __memoryBlockDevice_flush(Device* device) {
}