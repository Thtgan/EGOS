#if !defined(__DEVICES_CHAR_CHARDEVICE_H)
#define __DEVICES_CHAR_CHARDEVICE_H

typedef struct CharDeviceInitArgs CharDeviceInitArgs;
typedef struct CharDevice CharDevice;

#include<devices/device.h>
#include<kit/oop.h>
#include<kit/types.h>

typedef struct CharDeviceInitArgs {
    DeviceInitArgs          deviceInitArgs;
} CharDeviceInitArgs;

typedef struct CharDevice {
    Device                      device;
} CharDevice;

void charDevice_initStruct(CharDevice* device, CharDeviceInitArgs* args);

void charDevice_read(CharDevice* device, Index64 index, void* buffer, Size n);

void charDevice_write(CharDevice* device, Index64 index, const void* buffer, Size n);

void charDevice_flush(CharDevice* device);

#endif // __DEVICES_CHAR_CHARDEVICE_H
