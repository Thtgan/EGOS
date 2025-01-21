#include<devices/block/memoryBlockDevice.h>

#include<devices/block/blockDevice.h>
#include<devices/device.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<cstring.h>
#include<structs/singlyLinkedList.h>
#include<error.h>

static void __memoryBlockDevice_read(Device* device, Index64 blockIndex, void* buffer, Size n);

static void __memoryBlockDevice_write(Device* device, Index64 blockIndex, const void* buffer, Size n);

static void __memoryBlockDevice_flush(Device* device);

static DeviceOperations _memoryBlockDevice_deviceOperations = (DeviceOperations) {
    .read = __memoryBlockDevice_read,
    .write = __memoryBlockDevice_write,
    .flush = __memoryBlockDevice_flush
};

void memoryBlockDevice_initStruct(BlockDevice* device, void* region, Size size, ConstCstring name) {
    if (region == NULL) {
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
    memory_memset(region, 0, blockNum * BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);

    BlockDeviceInitArgs args = {
        .deviceInitArgs     = (DeviceInitArgs) {
            .id             = DEVICE_BUILD_ID(major, minor),
            .name           = name,
            .parent         = NULL,
            .granularity    = BLOCK_DEVICE_DEFAULT_BLOCK_SIZE_SHIFT,
            .capacity       = blockNum,
            .flags          = EMPTY_FLAGS,
            .operations     = &_memoryBlockDevice_deviceOperations,
            .specificInfo   = (Object)region
        },
    };

    blockDevice_initStruct(device, &args);
    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryBlockDevice_read(Device* device, Index64 blockIndex, void* buffer, Size n) {
    memory_memcpy(buffer, (void*)device->specificInfo + blockIndex * POWER_2(device->granularity), n * POWER_2(device->granularity));  //Just simple memcpy
}

static void __memoryBlockDevice_write(Device* device, Index64 blockIndex, const void* buffer, Size n) {
    memory_memcpy((void*)device->specificInfo + blockIndex * POWER_2(device->granularity), buffer, n * POWER_2(device->granularity));  //Just simple memcpy
}

static void __memoryBlockDevice_flush(Device* device) {
}