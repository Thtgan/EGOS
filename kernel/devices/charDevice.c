#include<devices/charDevice.h>

#include<devices/device.h>
#include<kit/oop.h>
#include<kit/types.h>

void charDevice_initStruct(CharDevice* device, CharDeviceInitArgs* args) {
    device_initStruct(&device->device, &args->deviceInitArgs);

    //TODO: Buffer for char device
}

void charDevice_read(CharDevice* device, Index64 index, void* buffer, Size n) {
    device_rawReadUnits(&device->device, index, buffer, n);
}

void charDevice_write(CharDevice* device, Index64 index, const void* buffer, Size n) {
    device_rawWriteUnits(&device->device, index, buffer, n);
}

void charDevice_flush(CharDevice* device) {
    device_rawFlush(&device->device);
}
