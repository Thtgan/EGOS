#include<devices/terminal/tty.h>

#include<devices/display/display.h>
#include<devices/terminal/virtualTTY.h>
#include<devices/terminal/bootTTY.h>
#include<kit/types.h>
#include<system/pageTable.h>
#include<error.h>

void teletype_initStruct(Teletype* tty, TeletypeOperations* operations) {
    tty->operations = operations;
}

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