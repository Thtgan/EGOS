#include<devices/pseudo.h>

#include<devices/charDevice.h>
#include<devices/device.h>
#include<devices/terminal/tty.h>
#include<print.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<error.h>

static void __pseudo_nullDevice_operations_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN);

static void __pseudo_nullDevice_operations_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN);

static void __pseudo_nullDevice_operations_flush(Device* device);

static DeviceOperations __pseudo_nullDevice_operations = (DeviceOperations) {
    .readUnits  = __pseudo_nullDevice_operations_readUnits,
    .writeUnits = __pseudo_nullDevice_operations_writeUnits,
    .flush      = __pseudo_nullDevice_operations_flush
};

static CharDevice _pseudo_nullDevice;
static TeletypeDevice _pseudo_teletypeDevice;

void pseudoDevice_init() {
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

    CharDeviceInitArgs args = (CharDeviceInitArgs) {
        .deviceInitArgs     = (DeviceInitArgs) {
            .id             = DEVICE_BUILD_ID(major, minor),
            .name           = "null",
            .parent         = NULL,
            .granularity    = 0,
            .capacity       = INFINITE,
            .flags          = EMPTY_FLAGS,
            .operations     = &__pseudo_nullDevice_operations
        },
    };

    charDevice_initStruct(&_pseudo_nullDevice, &args);
    device_registerDevice(&_pseudo_nullDevice.device);
    ERROR_GOTO_IF_ERROR(0);

    teletypeDevice_initStruct(&_pseudo_teletypeDevice, tty_getCurrentTTY());
    device_registerDevice(&_pseudo_teletypeDevice.device.device);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __pseudo_nullDevice_operations_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN) {
    PTR_TO_VALUE(8, buffer) = 0;
}

static void __pseudo_nullDevice_operations_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN) {
}

static void __pseudo_nullDevice_operations_flush(Device* device) {
}