#if !defined(__DEVICES_TERMINAL_BOOTTTY_H)
#define __DEVICES_TERMINAL_BOOTTTY_H

typedef struct BootTeletype BootTeletype;

#include<devices/terminal/tty.h>

typedef struct BootTeletype {
    Teletype tty;
    Uint8 printedRowNum;
    Uint16 lastRowLength;
} BootTeletype;

void bootTeletype_initStruct(BootTeletype* tty);

#endif // __DEVICES_TERMINAL_BOOTTTY_H
