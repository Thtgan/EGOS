#include<devices/block/memoryBlockDevice.h>

#include<devices/block/blockDevice.h>
#include<devices/device.h>
#include<error.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<cstring.h>
#include<structs/singlyLinkedList.h>

static Result __memoryBlockDevice_read(Device* device, Index64 blockIndex, void* buffer, Size n);

static Result __memoryBlockDevice_write(Device* device, Index64 blockIndex, const void* buffer, Size n);

static Result __memoryBlockDevice_flush(Device* device);

static DeviceOperations _memoryBlockDevice_deviceOperations = (DeviceOperations) {
    .read = __memoryBlockDevice_read,
    .write = __memoryBlockDevice_write,
    .flush = __memoryBlockDevice_flush
};

Result memoryBlockDevice_initStruct(BlockDevice* device, void* region, Size size, ConstCstring name) {
    if (region == NULL) {
        ERROR_CODE_SET(ERROR_CODE_OBJECT_ARGUMENT, ERROR_CODE_STATUS_IS_NULL);
        return RESULT_FAIL;
    }

    MajorDeviceID major = device_allocMajor();
    if (major == DEVICE_INVALID_ID) {
        return RESULT_FAIL;
    }

    MinorDeviceID minor = device_allocMinor(major);
    if (minor == DEVICE_INVALID_ID) {
        return RESULT_FAIL;
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

    return blockDevice_initStruct(device, &args);
}

static Result __memoryBlockDevice_read(Device* device, Index64 blockIndex, void* buffer, Size n) {
    memory_memcpy(buffer, (void*)device->specificInfo + blockIndex * POWER_2(device->granularity), n * POWER_2(device->granularity));  //Just simple memcpy
    return RESULT_SUCCESS;
}

static Result __memoryBlockDevice_write(Device* device, Index64 blockIndex, const void* buffer, Size n) {
    memory_memcpy((void*)device->specificInfo + blockIndex * POWER_2(device->granularity), buffer, n * POWER_2(device->granularity));  //Just simple memcpy
    return RESULT_SUCCESS;
}

static Result __memoryBlockDevice_flush(Device* device) {
    return RESULT_SUCCESS;
}