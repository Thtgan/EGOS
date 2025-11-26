#include<devices/terminal/tty.h>

#include<devices/display/display.h>
#include<devices/terminal/virtualTTY.h>
#include<devices/terminal/bootTTY.h>
#include<kit/types.h>
#include<system/pageTable.h>
#include<memory/memory.h>
#include<error.h>

void teletype_initStruct(Teletype* tty, TeletypeOperations* operations) {
    tty->operations = operations;
}

static void __teletypeDevice_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN);

static void __teletypeDevice_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN);

static void __teletypeDevice_flush(Device* device);

//TODO: Bad naming
static Teletype* _currentTTY;
static BootTeletype _bootTTY;
static VirtualTeletype _mainTTY;
static bool _isMainTTYinitialized;  //TODO: Remove this?

void tty_init() {
    _isMainTTYinitialized = false;
    bootTeletype_initStruct(&_bootTTY);
    _currentTTY = &_bootTTY.tty;
    
    return;
    ERROR_FINAL_BEGIN(0);
}

void tty_initVirtTTY() {
    virtualTeletype_initStruct(&_mainTTY, display_getCurrentContext(), 500);
    ERROR_GOTO_IF_ERROR(0);
    _isMainTTYinitialized = true;
    teletype_rawFlush(&_mainTTY.tty);   //Error passthrough
    _currentTTY = &_mainTTY.tty;

    return;
    ERROR_FINAL_BEGIN(0);
}

void tty_switchDisplayMode(DisplayMode mode) {
    display_switchMode(mode);
    virtualTeletype_updateDisplayContext(&_mainTTY, display_getCurrentContext());
    teletype_rawFlush(&_mainTTY.tty);   //Error passthrough

    return;
    ERROR_FINAL_BEGIN(0);
}

Teletype* tty_getCurrentTTY() {
    return _currentTTY;
}

static DeviceOperations _teletypeDevice_operations = {
    .readUnits = __teletypeDevice_readUnits,
    .writeUnits = __teletypeDevice_writeUnits,
    .flush = __teletypeDevice_flush
};

void teletypeDevice_initStruct(TeletypeDevice* device, Teletype* tty) {
    static int ttyCnt = 0;
    static MajorDeviceID majorID = DEVICE_INVALID_ID;
    char nameBuffer[8];
    
    if (majorID == DEVICE_INVALID_ID) {
        majorID = device_allocMajor();
    }

    MinorDeviceID minorID = device_allocMinor(majorID);
    memory_memset(nameBuffer, 0, sizeof(nameBuffer));   //TODO: Set tailing 0 in sprintf
    print_sprintf(nameBuffer, "tty%d", minorID);
    
    CharDeviceInitArgs args = {
        .deviceInitArgs = {
            .id = DEVICE_BUILD_ID(majorID, minorID),
            .name = nameBuffer,
            .parent = NULL,
            .granularity = 0,
            .flags = EMPTY_FLAGS,
            .operations = &_teletypeDevice_operations
        }
    };

    charDevice_initStruct(&device->device, &args);

    device->tty = tty;
}

static void __teletypeDevice_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN) {
    TeletypeDevice* teletypeDevice = HOST_POINTER(device, TeletypeDevice, device.device);
    Teletype* tty = teletypeDevice->tty;

    teletype_rawRead(tty, buffer, unitN);
}

static void __teletypeDevice_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN) {
    TeletypeDevice* teletypeDevice = HOST_POINTER(device, TeletypeDevice, device.device);
    Teletype* tty = teletypeDevice->tty;

    teletype_rawWrite(tty, buffer, unitN);
}

static void __teletypeDevice_flush(Device* device) {
    TeletypeDevice* teletypeDevice = HOST_POINTER(device, TeletypeDevice, device.device);
    Teletype* tty = teletypeDevice->tty;

    teletype_rawFlush(tty);
}