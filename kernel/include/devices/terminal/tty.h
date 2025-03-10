#if !defined(__DEVICES_TERMINAL_TTY_H)
#define __DEVICES_TERMINAL_TTY_H

typedef struct Teletype Teletype;
typedef struct TeletypeOperations TeletypeOperations;

#include<devices/display/display.h>
#include<kit/types.h>

typedef struct Teletype {
    TeletypeOperations* operations;
} Teletype;

typedef struct TeletypeOperations {
    Size (*read)(Teletype* tty, void* buffer, Size n);
    Size (*write)(Teletype* tty, const void* buffer, Size n);
    void (*flush)(Teletype* tty);
} TeletypeOperations;

void teletype_initStruct(Teletype* tty, TeletypeOperations* operations);

static inline Size teletype_rawRead(Teletype* tty, void* buffer, Size n) {
    return tty->operations->read(tty, buffer, n);
}

static inline Size teletype_rawWrite(Teletype* tty, const void* buffer, Size n) {
    return tty->operations->write(tty, buffer, n);
}

static inline void teletype_rawFlush(Teletype* tty) {
    tty->operations->flush(tty);
}

void tty_init();

void tty_initVirtTTY();

void tty_switchDisplayMode(DisplayMode mode);

Teletype* tty_getCurrentTTY();

#endif // __DEVICES_TERMINAL_TTY_H
