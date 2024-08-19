#include<devices/char/charDevice.h>

#include<devices/device.h>
#include<kit/oop.h>
#include<kit/types.h>

Result charDevice_initStruct(CharDevice* device, CharDeviceInitArgs* args) {
    device_initStruct(&device->device, &args->deviceInitArgs);

    //TODO: Buffer for char device

    return RESULT_SUCCESS;
}

Result charDevice_read(CharDevice* device, Index64 index, void* buffer, Size n) {
    return device_rawRead(&device->device, index, buffer, n);
}

Result charDevice_write(CharDevice* device, Index64 index, const void* buffer, Size n) {
    return device_rawWrite(&device->device, index, buffer, n);
}

Result charDevice_flush(CharDevice* device) {
    return device_rawFlush(&device->device);
}
