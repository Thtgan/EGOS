#include<devices/terminal/tty.h>

#include<devices/display/display.h>
#include<devices/terminal/virtualTTY.h>
#include<devices/terminal/bootTTY.h>
#include<kit/types.h>
#include<system/pageTable.h>
#include<memory/memory.h>
#include<error.h>
#include<print.h>

void teletype_initStruct(Teletype* tty, TeletypeOperations* operations) {
    tty->operations = operations;
}

static void __teletypeDevice_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN);

static void __teletypeDevice_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN);

static void __teletypeDevice_flush(Device* device);

static int __tty_print(ConstCstring buffer, Size n, Object arg);

//TODO: Bad naming
static Teletype* _currentTTY;
static BootTeletype _bootTTY;
static VirtualTeletype _mainTTY;
static bool _isMainTTYinitialized;  //TODO: Remove this?
PrintHandler _tty_printHandler;

void tty_init() {
    _isMainTTYinitialized = false;
    bootTeletype_initStruct(&_bootTTY);
    _currentTTY = &_bootTTY.tty;

    _tty_printHandler.print = __tty_print;
    _tty_printHandler.arg = (Object)_currentTTY;
    
    return;
    ERROR_FINAL_BEGIN(0);
}

void tty_initVirtTTY() {
    virtualTeletype_initStruct(&_mainTTY, display_getCurrentContext(), 500);
    ERROR_GOTO_IF_ERROR(0);
    _isMainTTYinitialized = true;
    teletype_rawFlush(&_mainTTY.tty);   //Error passthrough
    _currentTTY = &_mainTTY.tty;

    _tty_printHandler.arg = (Object)_currentTTY;

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

PrintHandler* tty_getPrintHandler() {
    return &_tty_printHandler;
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
    // print_sprintf(nameBuffer, "tty%d", minorID);
    print_snprintf(nameBuffer, sizeof(nameBuffer), "tty%d", minorID);
    
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

static int __tty_print(ConstCstring buffer, Size n, Object arg) {
    Teletype* tty = (Teletype*)arg;
    teletype_rawWrite(tty, buffer, n);
    teletype_rawFlush(tty);

    return n;
}