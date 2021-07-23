#include"keyboard.h"

#include<drivers/interrupt/IDT.h>
#include<drivers/basicPrint.h>
#include<drivers/vgaTextMode.h>

#include<stdint.h>
#include<stdbool.h>

#define SCAN_CODE_TRIM(SCANCODE)                (SCANCODE & 0x7F)
#define SCANCODE_PRESS(SCANCODE)                (SCANCODE & 0x7F)
#define SCANCODE_RELEASE(SCANCODE)              (SCANCODE | 0x80)

#define KEYMAP_ENTRY(ASCII, SHIFT_ASCII, FLAGS) (((uint16_t)(FLAGS) << 16) | ((uint8_t)(SHIFT_ASCII) << 8) | (uint8_t)(ASCII))
#define ENTRY_ASCII(ENTRY)                      (ENTRY & 0xFF)
#define ENTRY_ALT_ASCII(ENTRY)                  ((ENTRY >> 8) & 0xFF)
#define ENTRY_FLAGS(ENTRY)                      ((ENTRY >> 16) & 0xFFFF)
#define IS_ASCII                                1
#define HAS_ALT_ASCII                           2
#define IS_ALPHA                                4
#define IS_CONTROL                              8
#define IS_KEYPAD                               16
#define IS_FUNCTION                             32
#define IS_CTRL                                 64
#define IS_SHIFT                                128
#define IS_ALT                                  256
#define IS_CAPSLOCK                             512
#define IS_ENTER                                1024

static bool _pressed[128];
static bool _capslock = false;

__attribute__((interrupt))
void keyboardDebugInterruptHandler(InterruptFrame* frame)
{
    setCursorPosition(0, 0);
    printHex64(frame->rip);
    putchar('\n');
    printHex64(frame->cs);
    putchar('\n');
    printHex64(frame->cpuFlags);
    putchar('\n');
    printHex64(frame->stackPtr);
    putchar('\n');
    printHex64(frame->stackSegment);
    putchar('\n');
    printHex8(__readScanCode());
    putchar('\n');
    EOI();
}

static uint8_t __readScanCode()
{
    return inb(0x60);
}

static uint32_t _keyMap[256] = {
    KEYMAP_ENTRY('\0',  '\0',   0),                                     //0x00 -- NULL
    KEYMAP_ENTRY('\0',  '\0',   0),                                     //0x01 -- ESC
    KEYMAP_ENTRY('1',   '!',    IS_ASCII | HAS_ALT_ASCII),              //0x02 -- 1
    KEYMAP_ENTRY('2',   '@',    IS_ASCII | HAS_ALT_ASCII),              //0x03 -- 2
    KEYMAP_ENTRY('3',   '#',    IS_ASCII | HAS_ALT_ASCII),              //0x04 -- 3
    KEYMAP_ENTRY('4',   '$',    IS_ASCII | HAS_ALT_ASCII),              //0x05 -- 4
    KEYMAP_ENTRY('5',   '%',    IS_ASCII | HAS_ALT_ASCII),              //0x06 -- 5
    KEYMAP_ENTRY('6',   '^',    IS_ASCII | HAS_ALT_ASCII),              //0x07 -- 6
    KEYMAP_ENTRY('7',   '&',    IS_ASCII | HAS_ALT_ASCII),              //0x08 -- 7
    KEYMAP_ENTRY('8',   '*',    IS_ASCII | HAS_ALT_ASCII),              //0x09 -- 8
    KEYMAP_ENTRY('9',   '(',    IS_ASCII | HAS_ALT_ASCII),              //0x0A -- 9
    KEYMAP_ENTRY('0',   ')',    IS_ASCII | HAS_ALT_ASCII),              //0x0B -- 0
    KEYMAP_ENTRY('-',   '_',    IS_ASCII | HAS_ALT_ASCII),              //0x0C -- HYPHEN
    KEYMAP_ENTRY('=',   '+',    IS_ASCII | HAS_ALT_ASCII),              //0x0D -- EQUAL
    KEYMAP_ENTRY('\b',  '\0',   IS_ASCII),                              //0x0E -- BACKSPACE
    KEYMAP_ENTRY('\t',  '\0',   IS_ASCII),                              //0x0F -- TAB
    KEYMAP_ENTRY('q',   'Q',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x10 -- Q
    KEYMAP_ENTRY('w',   'W',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x11 -- W
    KEYMAP_ENTRY('e',   'E',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x12 -- E
    KEYMAP_ENTRY('r',    'R',   IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x13 -- R
    KEYMAP_ENTRY('t',   'T',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x14 -- T
    KEYMAP_ENTRY('y',   'Y',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x15 -- Y
    KEYMAP_ENTRY('u',   'U',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x16 -- U
    KEYMAP_ENTRY('i',   'I',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x17 -- I
    KEYMAP_ENTRY('o',   'O',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x18 -- O
    KEYMAP_ENTRY('p',   'P',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x19 -- P
    KEYMAP_ENTRY('[',   '{',    IS_ASCII | HAS_ALT_ASCII),              //0x1A -- LEFT_SQUARE_BARCKET
    KEYMAP_ENTRY(']',   '}',    IS_ASCII | HAS_ALT_ASCII),              //0x1B -- LEFT_SQUARE_BARCKET
    KEYMAP_ENTRY('\n',  '\0',   IS_ASCII | IS_ENTER),                   //0x1C -- ENTER
    KEYMAP_ENTRY('\0',  '\0',   IS_CTRL),                               //0x1D -- LEFT_CTRL
    KEYMAP_ENTRY('a',   'A',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x1E -- A
    KEYMAP_ENTRY('s',   'S',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x1F -- S
    KEYMAP_ENTRY('d',   'D',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x20 -- D
    KEYMAP_ENTRY('f',   'F',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x21 -- F
    KEYMAP_ENTRY('g',   'G',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x22 -- G
    KEYMAP_ENTRY('h',   'H',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x23 -- H
    KEYMAP_ENTRY('j',   'J',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x24 -- J
    KEYMAP_ENTRY('k',   'K',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x25 -- K
    KEYMAP_ENTRY('l',   'L',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x26 -- L
    KEYMAP_ENTRY(';',   ':',    IS_ASCII | HAS_ALT_ASCII),              //0x27 -- SEMICOLON
    KEYMAP_ENTRY('\'',  '\"',   IS_ASCII | HAS_ALT_ASCII),              //0x28 -- SINGLE_QUOTE
    KEYMAP_ENTRY('`',   '~',    IS_ASCII | HAS_ALT_ASCII),              //0x29 -- BACKTICK
    KEYMAP_ENTRY('\0',  '\0',   IS_SHIFT),                              //0x2A -- LEFT_SHIFT
    KEYMAP_ENTRY('\\',  '|',    IS_ASCII | HAS_ALT_ASCII),              //0x2B -- BACKWARD_SLASH
    KEYMAP_ENTRY('z',   'Z',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x2C -- Z
    KEYMAP_ENTRY('x',   'X',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x2D -- X
    KEYMAP_ENTRY('c',   'C',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x2E -- C
    KEYMAP_ENTRY('v',   'V',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x2F -- V
    KEYMAP_ENTRY('b',   'B',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x30 -- B
    KEYMAP_ENTRY('n',   'N',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x31 -- N
    KEYMAP_ENTRY('m',   'M',    IS_ASCII | HAS_ALT_ASCII | IS_ALPHA),   //0x32 -- M
    KEYMAP_ENTRY(',',   '<',    IS_ASCII | HAS_ALT_ASCII),              //0x33 -- COMMA
    KEYMAP_ENTRY('.',   '>',    IS_ASCII | HAS_ALT_ASCII),              //0x34 -- PERIOD
    KEYMAP_ENTRY('/',   '?',    IS_ASCII | HAS_ALT_ASCII),              //0x35 -- SLASH
    KEYMAP_ENTRY('\0',  '\0',   IS_SHIFT),                              //0x36 -- RIGHT_SHIFT
    KEYMAP_ENTRY('*',   '\0',   IS_ASCII | IS_KEYPAD),                  //0x37 -- KEYPAD_ASTERISK
    KEYMAP_ENTRY('\0',  '\0',   IS_ALT),                                //0x38 -- LEFT_ALT
    KEYMAP_ENTRY(' ',   '\0',   IS_ASCII),                              //0x39 -- SPACE
    KEYMAP_ENTRY('\0',  '\0',   IS_CAPSLOCK),                           //0x3A -- CAPSLOCK
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x3B -- F1
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x3C -- F2
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x3D -- F3
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x3E -- F4
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x3F -- F5
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x40 -- F6
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x41 -- F7
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x42 -- F8
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x43 -- F9
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x44 -- F10
    KEYMAP_ENTRY('\0',  '\0',   0),                                     //0x45 -- NUMBERLOCK
    KEYMAP_ENTRY('\0',  '\0',   0),                                     //0x46 -- SCROLLLOCK
    KEYMAP_ENTRY('7',   '\0',   IS_KEYPAD),                             //0x47 -- KEYPAD_7
    KEYMAP_ENTRY('8',   '\0',   IS_KEYPAD),                             //0x48 -- KEYPAD_8
    KEYMAP_ENTRY('0',   '\0',   IS_KEYPAD),                             //0x49 -- KEYPAD_9
    KEYMAP_ENTRY('-',   '\0',   IS_ASCII | IS_KEYPAD),                  //0x4A -- KEYPAD_MINUS
    KEYMAP_ENTRY('4',   '\0',   IS_KEYPAD),                             //0x4B -- KEYPAD_4
    KEYMAP_ENTRY('5',   '\0',   IS_KEYPAD),                             //0x4C -- KEYPAD_5
    KEYMAP_ENTRY('6',   '\0',   IS_KEYPAD),                             //0x4D -- KEYPAD_6
    KEYMAP_ENTRY('+',   '\0',   IS_ASCII | IS_KEYPAD),                  //0x4E -- KEYPAD_PLUS
    KEYMAP_ENTRY('1',   '\0',   IS_KEYPAD),                             //0x4F -- KEYPAD_1
    KEYMAP_ENTRY('2',   '\0',   IS_KEYPAD),                             //0x50 -- KEYPAD_2
    KEYMAP_ENTRY('3',   '\0',   IS_KEYPAD),                             //0x51 -- KEYPAD_3
    KEYMAP_ENTRY('0',   '\0',   IS_KEYPAD),                             //0x52 -- KEYPAD_0
    KEYMAP_ENTRY('.',   '\0',   IS_KEYPAD),                             //0x53 -- KEYPAD_PERIOD
    KEYMAP_ENTRY('\0',  '\0',   0),                                     //0x54 -- NULL
    KEYMAP_ENTRY('\0',  '\0',   0),                                     //0x55 -- NULL
    KEYMAP_ENTRY('\0',  '\0',   0),                                     //0x56 -- NULL
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x57 -- F11
    KEYMAP_ENTRY('\0',  '\0',   IS_FUNCTION),                           //0x58 -- F12
};

__attribute__((interrupt))
void keyboardInterruptHandler(InterruptFrame* frame)
{
    uint8_t scancode = __readScanCode();
    uint8_t key = SCAN_CODE_TRIM(scancode);
    _pressed[key] = (SCANCODE_PRESS(scancode) == scancode);

    if (!_pressed[KEY_CAPSLOCK] && key == KEY_CAPSLOCK)
        _capslock ^= true;
    
    else if (_pressed[key] && (ENTRY_FLAGS(_keyMap[key]) & IS_ASCII) != 0)
        putchar(__toASCII(key));

    else if (_pressed[key] && (ENTRY_FLAGS(_keyMap[key]) & IS_KEYPAD) != 0)
    {
        const VGAStatus* vgaStatus = getVGAStatus();
        int row = vgaStatus->cursorPosition / VGA_WIDTH, col = vgaStatus->cursorPosition % VGA_WIDTH;
        switch (key)
        {
        case KEY_KEYPAD_1:
            setCursorPosition(row, VGA_WIDTH - 1);
            break;
        case KEY_KEYPAD_2:
            setCursorPosition(row + 1, col);
            break;
        case KEY_KEYPAD_3:
            //Not implemented
            break;
        case KEY_KEYPAD_4:
            setCursorPosition(row, col - 1);
            break;
        case KEY_KEYPAD_5:
            break;
        case KEY_KEYPAD_6:
            setCursorPosition(row, col + 1);
            break;
        case KEY_KEYPAD_7:
            setCursorPosition(row, 0);
            break;
        case KEY_KEYPAD_8:
            setCursorPosition(row - 1, col);
            break;
        case KEY_KEYPAD_9:
            //Not implemented
            break;
        default:
            break;
        }
    }

    EOI();
}

static uint8_t __toASCII(uint8_t key)
{
    uint8_t ch = ENTRY_ASCII(_keyMap[key]);
    if (((ENTRY_FLAGS(_keyMap[key]) & HAS_ALT_ASCII) != 0) && (_pressed[KEY_LEFT_SHIFT] || _pressed[KEY_RIGHT_SHIFT]))
        ch = ENTRY_ALT_ASCII(_keyMap[key]);
    if (((ENTRY_FLAGS(_keyMap[key]) & IS_ALPHA) != 0) && _capslock)
        ch = (_pressed[KEY_LEFT_SHIFT] || _pressed[KEY_RIGHT_SHIFT]) ? ch :  ENTRY_ALT_ASCII(_keyMap[key]);
    return ch;
}

bool getCapslock()
{
    return _capslock;
}

bool isPressed(uint8_t key)
{
    return _pressed[key];
}