#include<devices/pseudo.h>

#include<devices/device.h>
#include<fs/fsStructs.h>
#include<print.h>
#include<kit/types.h>
#include<kit/util.h>

static Result __devfs_fsEntry_nullRead(fsEntry* entry, void* buffer, Size n);

static Result __devfs_fsEntry_nullWrite(fsEntry* entry, const void* buffer, Size n);

static Index64 __devfs_fsEntry_nullSeek(fsEntry* entry, Index64 seekTo);

static Result  __devfs_fsEntry_nullResize(fsEntry* entry, Size newSizeInByte);

fsEntryOperations _nullDeviceOperations = {
    .read = __devfs_fsEntry_nullRead,
    .write = __devfs_fsEntry_nullWrite,
    .seek = __devfs_fsEntry_nullSeek,
    .resize = __devfs_fsEntry_nullResize
};

Device _nullDevice;

Result pseudoDevice_init() {
    MajorDeviceID major = device_allocMajor();
    MinorDeviceID minor = device_allocMinor(major);
    device_initStruct(&_nullDevice, DEVICE_ID(major, minor), "null", NULL, &_nullDeviceOperations);
    device_registerDevice(&_nullDevice);

    return RESULT_SUCCESS;
}

static Result __devfs_fsEntry_nullRead(fsEntry* entry, void* buffer, Size n) {
    PTR_TO_VALUE(8, buffer) = 0;
    return RESULT_SUCCESS;
}

static Result __devfs_fsEntry_nullWrite(fsEntry* entry, const void* buffer, Size n) {
    printf(TERMINAL_LEVEL_OUTPUT, "%u-%s\n", n, buffer);
    return RESULT_SUCCESS;
}

static Index64 __devfs_fsEntry_nullSeek(fsEntry* entry, Index64 seekTo) {
    return 0;
}

static Result  __devfs_fsEntry_nullResize(fsEntry* entry, Size newSizeInByte) {
    return RESULT_SUCCESS;
}
