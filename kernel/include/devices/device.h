#if !defined(__DEVICE_H)
#define __DEVICE_H

#include<devices/block/blockDevice.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<structs/RBtree.h>

typedef Uint32 DeviceID;
typedef Uint32 MajorDeviceID;
typedef Uint32 MinorDeviceID;

#define DEVICE_ID_MAJOR_SHIFT 20

#define INVALID_DEVICE_ID           INVALID_INDEX
#define DEVICE_ID(__MAJOR, __MINOR) (VAL_OR(VAL_LEFT_SHIFT((__MAJOR), DEVICE_ID_MAJOR_SHIFT), (__MINOR)))
#define MAJOR_FROM_DEVICE_ID(__ID)  EXTRACT_VAL((__ID), 32, DEVICE_ID_MAJOR_SHIFT, 32)
#define MINOR_FROM_DEVICE_ID(__ID)  EXTRACT_VAL((__ID), 32, 0, DEVICE_ID_MAJOR_SHIFT)

typedef struct {
    DeviceID        id;
    char            name[32];
    BlockDevice*    dev;
    void*           operations; //TODO: It is fsEntryOperations, Fix this reference problem
    RBtreeNode      deviceTreeNode;
} Device;

void device_initStruct(Device* device, DeviceID id, ConstCstring name, BlockDevice* dev, void* operations);

Result device_init();

MajorDeviceID device_allocMajor();

Result device_releaseMajor(MajorDeviceID major);

MinorDeviceID device_allocMinor(MajorDeviceID major);

Result device_registerDevice(Device* device);

Result device_unregisterDevice(DeviceID id);

Device* device_getDevice(DeviceID id);

MajorDeviceID device_iterateMajor(MajorDeviceID current);

Device* device_iterateMinor(MajorDeviceID major, MinorDeviceID current);

#endif // __DEVICE_H
