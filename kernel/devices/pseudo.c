#include<devices/pseudo.h>

#include<devices/device.h>
#include<fs/fsStructs.h>
#include<print.h>
#include<kit/types.h>
#include<kit/util.h>

static Result __pseudo_nullDevice_read(fsEntry* entry, void* buffer, Size n);

static Result __pseudo_nullDevice_Write(fsEntry* entry, const void* buffer, Size n);

static Index64 __pseudo_nullDevice_Seek(fsEntry* entry, Index64 seekTo);

static Result  __pseudo_nullDevice_Resize(fsEntry* entry, Size newSizeInByte);

fsEntryOperations _pseudo_nullDevice_fsEntryOperations = {
    .read = __pseudo_nullDevice_read,
    .write = __pseudo_nullDevice_Write,
    .seek = __pseudo_nullDevice_Seek,
    .resize = __pseudo_nullDevice_Resize
};

static Result __pseudo_stdoutDevice_read(fsEntry* entry, void* buffer, Size n);

static Result __pseudo_stdoutDevice_Write(fsEntry* entry, const void* buffer, Size n);

static Index64 __pseudo_stdoutDevice_Seek(fsEntry* entry, Index64 seekTo);

static Result  __pseudo_stdoutDevice_Resize(fsEntry* entry, Size newSizeInByte);

fsEntryOperations _pseudo_stdoutDevice_fsEntryOperations = {
    .read = __pseudo_stdoutDevice_read,
    .write = __pseudo_stdoutDevice_Write,
    .seek = __pseudo_stdoutDevice_Seek,
    .resize = __pseudo_stdoutDevice_Resize
};

static Device _pseudo_nullDevice;
static Device _pseudo_stdoutDevice;

Result pseudoDevice_init() {
    MajorDeviceID major = device_allocMajor();
    MinorDeviceID minor = device_allocMinor(major);
    device_initStruct(&_pseudo_nullDevice, DEVICE_ID(major, minor), "null", NULL, &_pseudo_nullDevice_fsEntryOperations);
    device_registerDevice(&_pseudo_nullDevice);

    minor = device_allocMinor(major);
    device_initStruct(&_pseudo_stdoutDevice, DEVICE_ID(major, minor), "stdout", NULL, &_pseudo_stdoutDevice_fsEntryOperations);
    device_registerDevice(&_pseudo_stdoutDevice);

    return RESULT_SUCCESS;
}

static Result __pseudo_nullDevice_read(fsEntry* entry, void* buffer, Size n) {
    PTR_TO_VALUE(8, buffer) = 0;
    return RESULT_SUCCESS;
}

static Result __pseudo_nullDevice_Write(fsEntry* entry, const void* buffer, Size n) {
    return RESULT_SUCCESS;
}

static Index64 __pseudo_nullDevice_Seek(fsEntry* entry, Index64 seekTo) {
    return 0;
}

static Result  __pseudo_nullDevice_Resize(fsEntry* entry, Size newSizeInByte) {
    return RESULT_SUCCESS;
}

static Result __pseudo_stdoutDevice_read(fsEntry* entry, void* buffer, Size n) {
    return RESULT_SUCCESS;
}

static Result __pseudo_stdoutDevice_Write(fsEntry* entry, const void* buffer, Size n) {
    print_printf(TERMINAL_LEVEL_OUTPUT, buffer);
    return RESULT_SUCCESS;
}

static Index64 __pseudo_stdoutDevice_Seek(fsEntry* entry, Index64 seekTo) {
    return 0;
}

static Result  __pseudo_stdoutDevice_Resize(fsEntry* entry, Size newSizeInByte) {
    return RESULT_SUCCESS;
}