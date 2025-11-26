#if !defined(__DEVICES_DEVICE_H)
#define __DEVICES_DEVICE_H

typedef struct Device Device;
typedef struct DeviceInitArgs DeviceInitArgs;
typedef struct DeviceOperations DeviceOperations;

#include<kit/types.h>

typedef Uint32 DeviceID;
typedef Uint32 MajorDeviceID;
typedef Uint32 MinorDeviceID;

#include<kit/bit.h>
#include<kit/oop.h>
#include<structs/RBtree.h>
#include<structs/singlyLinkedList.h>

#define DEVICE_ID_MAJOR_SHIFT   20
#define DEVICE_NAME_MAX_LENGTH  31

#define DEVICE_INVALID_ID                   INVALID_INDEX32
#define DEVICE_BUILD_ID(__MAJOR, __MINOR)   (VAL_OR(VAL_LEFT_SHIFT((__MAJOR), DEVICE_ID_MAJOR_SHIFT), (__MINOR)))
#define DEVICE_MAJOR_FROM_ID(__ID)          EXTRACT_VAL((__ID), 32, DEVICE_ID_MAJOR_SHIFT, 32)
#define DEVICE_MINOR_FROM_ID(__ID)          EXTRACT_VAL((__ID), 32, 0, DEVICE_ID_MAJOR_SHIFT)

void device_init();

MajorDeviceID device_allocMajor();

void device_releaseMajor(MajorDeviceID major);

MinorDeviceID device_allocMinor(MajorDeviceID major);

typedef struct Device {
    DeviceID                    id;
    char                        name[DEVICE_NAME_MAX_LENGTH + 1];
    Device*                     parent;
    Size                        granularity;    //In Shift, 0 means this is a character device
    Size                        capacity;       //In Unit of granuality, real capacity = capacity * POWER_2(granularity)
#define DEVICE_FLAGS_READONLY   FLAG8(0)
#define DEVICE_FLAGS_BUFFERED   FLAG8(1)
    Flags8                      flags;

    Uint8                       childNum;
    SinglyLinkedList            children;
    SinglyLinkedListNode        childNode;
    
    RBtreeNode                  deviceTreeNode;

    DeviceOperations*           operations;
} Device;

static inline bool device_isBlockDevice(Device* device) {
    return device->granularity != 0;
}

typedef struct DeviceInitArgs {
    DeviceID            id;
    ConstCstring        name;
    Device*             parent;
    Size                granularity;
    Size                capacity;
    Flags8              flags;
    DeviceOperations*   operations;
} DeviceInitArgs;

void device_initStruct(Device* device, DeviceInitArgs* args);

void device_registerDevice(Device* device);

void device_unregisterDevice(DeviceID id);

Device* device_getDevice(DeviceID id);

MajorDeviceID device_iterateMajor(MajorDeviceID current);

Device* device_iterateMinor(MajorDeviceID major, MinorDeviceID current);

void device_read(Device* device, Index64 begin, void* buffer, Size n);

void device_write(Device* device, Index64 begin, const void* buffer, Size n);

typedef struct DeviceOperations {
    void (*readUnits)(Device* device, Index64 unitIndex, void* buffer, Size unitN);

    void (*writeUnits)(Device* device, Index64 unitIndex, const void* buffer, Size unitN);

    void (*flush)(Device* device);
} DeviceOperations;

static inline void device_rawReadUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN) {
    device->operations->readUnits(device, unitIndex, buffer, unitN);
}

static inline void device_rawWriteUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN) {
    device->operations->writeUnits(device, unitIndex, buffer, unitN);
}

static inline void device_rawFlush(Device* device) {
    device->operations->flush(device);
}

#endif // __DEVICES_DEVICE_H
