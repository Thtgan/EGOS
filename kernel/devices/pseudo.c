#include<devices/pseudo.h>

#include<devices/char/charDevice.h>
#include<devices/device.h>
#include<print.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>

static Result __pseudo_nullDevice_operations_read(Device* device, Index64 index, void* buffer, Size n);

static Result __pseudo_nullDevice_operations_write(Device* device, Index64 index, const void* buffer, Size n);

static Result __pseudo_nullDevice_operations_flush(Device* device);

static DeviceOperations __pseudo_nullDevice_operations = (DeviceOperations) {
    .read   = __pseudo_nullDevice_operations_read,
    .write  = __pseudo_nullDevice_operations_write,
    .flush  = __pseudo_nullDevice_operations_flush
};

static Result __pseudo_stdoutDevice_operations_read(Device* device, Index64 index, void* buffer, Size n);

static Result __pseudo_stdoutDevice_operations_write(Device* device, Index64 index, const void* buffer, Size n);

static Result __pseudo_stdoutDevice_operations_flush(Device* device);

static DeviceOperations __pseudo_stdoutDevice_operations = (DeviceOperations) {
    .read   = __pseudo_stdoutDevice_operations_read,
    .write  = __pseudo_stdoutDevice_operations_write,
    .flush  = __pseudo_stdoutDevice_operations_flush
};

static CharDevice _pseudo_nullDevice;
static CharDevice _pseudo_stdoutDevice;

Result pseudoDevice_init() {
    MajorDeviceID major = device_allocMajor();
    if (major == DEVICE_INVALID_ID) {
        return RESULT_FAIL;
    }

    MinorDeviceID minor = device_allocMinor(major);
    if (minor == DEVICE_INVALID_ID) {
        return RESULT_FAIL;
    }

    CharDeviceInitArgs args = (CharDeviceInitArgs) {
        .deviceInitArgs     = (DeviceInitArgs) {
            .id             = DEVICE_BUILD_ID(major, minor),
            .name           = "null",
            .parent         = NULL,
            .granularity    = 0,
            .capacity       = INFINITE,
            .flags          = EMPTY_FLAGS,
            .operations     = &__pseudo_nullDevice_operations,
            .specificInfo   = OBJECT_NULL
        },
    };

    if (charDevice_initStruct(&_pseudo_nullDevice, &args) == RESULT_FAIL || device_registerDevice(&_pseudo_nullDevice.device) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    minor = device_allocMinor(major);
    if (minor == DEVICE_INVALID_ID) {
        return RESULT_FAIL;
    }

    args = (CharDeviceInitArgs) {
        .deviceInitArgs     = (DeviceInitArgs) {
            .id             = DEVICE_BUILD_ID(major, minor),
            .name           = "stdout",
            .parent         = NULL,
            .granularity    = 0,
            .capacity       = INFINITE,
            .operations     = &__pseudo_stdoutDevice_operations,
            .flags          = EMPTY_FLAGS,
            .specificInfo   = OBJECT_NULL
        },
    };

    if (charDevice_initStruct(&_pseudo_stdoutDevice, &args) == RESULT_FAIL || device_registerDevice(&_pseudo_stdoutDevice.device) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __pseudo_nullDevice_operations_read(Device* device, Index64 index, void* buffer, Size n) {
    PTR_TO_VALUE(8, buffer) = 0;
    return RESULT_SUCCESS;
}

static Result __pseudo_nullDevice_operations_write(Device* device, Index64 index, const void* buffer, Size n) {
    return RESULT_SUCCESS;
}

static Result __pseudo_nullDevice_operations_flush(Device* device) {
    return RESULT_SUCCESS;
}

static Result __pseudo_stdoutDevice_operations_read(Device* device, Index64 index, void* buffer, Size n) {
    return RESULT_SUCCESS;
}

static Result __pseudo_stdoutDevice_operations_write(Device* device, Index64 index, const void* buffer, Size n) {
    print_printf(TERMINAL_LEVEL_OUTPUT, buffer);
    return RESULT_SUCCESS;
}

static Result __pseudo_stdoutDevice_operations_flush(Device* device) {
    return RESULT_SUCCESS;
}