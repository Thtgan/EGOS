#include<devices/keyboard/keyboard.h>

#include<devices/terminal/terminal.h>
#include<devices/terminal/terminalSwitch.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<real/ports/keyboard.h>
#include<real/simpleAsmLines.h>
#include<result.h>

__attribute__((aligned(4)))
static KeyboardKeyEntry _keyboard_keyEntries[128] = {
    KEYBOARD_KEY_ENTRY('\0', '\0',   0),                            //0x00 -- NULL
    KEYBOARD_KEY_ENTRY('\0', '\0',   0),                            //0x01 -- ESC
    KEYBOARD_KEY_ENTRY('1',  '!',    ASCII | ALT_ASCII),            //0x02 -- 1
    KEYBOARD_KEY_ENTRY('2',  '@',    ASCII | ALT_ASCII),            //0x03 -- 2
    KEYBOARD_KEY_ENTRY('3',  '#',    ASCII | ALT_ASCII),            //0x04 -- 3
    KEYBOARD_KEY_ENTRY('4',  '$',    ASCII | ALT_ASCII),            //0x05 -- 4
    KEYBOARD_KEY_ENTRY('5',  '%',    ASCII | ALT_ASCII),            //0x06 -- 5
    KEYBOARD_KEY_ENTRY('6',  '^',    ASCII | ALT_ASCII),            //0x07 -- 6
    KEYBOARD_KEY_ENTRY('7',  '&',    ASCII | ALT_ASCII),            //0x08 -- 7
    KEYBOARD_KEY_ENTRY('8',  '*',    ASCII | ALT_ASCII),            //0x09 -- 8
    KEYBOARD_KEY_ENTRY('9',  '(',    ASCII | ALT_ASCII),            //0x0A -- 9
    KEYBOARD_KEY_ENTRY('0',  ')',    ASCII | ALT_ASCII),             //0x0B -- 0
    KEYBOARD_KEY_ENTRY('-',  '_',    ASCII | ALT_ASCII),             //0x0C -- HYPHEN
    KEYBOARD_KEY_ENTRY('=',  '+',    ASCII | ALT_ASCII),             //0x0D -- EQUAL
    KEYBOARD_KEY_ENTRY('\b', '\0',   ASCII),                         //0x0E -- BACKSPACE
    KEYBOARD_KEY_ENTRY('\t', '\0',   ASCII),                         //0x0F -- TAB
    KEYBOARD_KEY_ENTRY('q',  'Q',    ASCII | ALT_ASCII | ALPHA),     //0x10 -- Q
    KEYBOARD_KEY_ENTRY('w',  'W',    ASCII | ALT_ASCII | ALPHA),     //0x11 -- W
    KEYBOARD_KEY_ENTRY('e',  'E',    ASCII | ALT_ASCII | ALPHA),     //0x12 -- E
    KEYBOARD_KEY_ENTRY('r',  'R',    ASCII | ALT_ASCII | ALPHA),     //0x13 -- R
    KEYBOARD_KEY_ENTRY('t',  'T',    ASCII | ALT_ASCII | ALPHA),     //0x14 -- T
    KEYBOARD_KEY_ENTRY('y',  'Y',    ASCII | ALT_ASCII | ALPHA),     //0x15 -- Y
    KEYBOARD_KEY_ENTRY('u',  'U',    ASCII | ALT_ASCII | ALPHA),     //0x16 -- U
    KEYBOARD_KEY_ENTRY('i',  'I',    ASCII | ALT_ASCII | ALPHA),     //0x17 -- I
    KEYBOARD_KEY_ENTRY('o',  'O',    ASCII | ALT_ASCII | ALPHA),     //0x18 -- O
    KEYBOARD_KEY_ENTRY('p',  'P',    ASCII | ALT_ASCII | ALPHA),     //0x19 -- P
    KEYBOARD_KEY_ENTRY('[',  '{',    ASCII | ALT_ASCII),             //0x1A -- LEFT_SQUARE_BARCKET
    KEYBOARD_KEY_ENTRY(']',  '}',    ASCII | ALT_ASCII),             //0x1B -- LEFT_SQUARE_BARCKET
    KEYBOARD_KEY_ENTRY('\n', '\0',   ASCII | ENTER),                 //0x1C -- ENTER
    KEYBOARD_KEY_ENTRY('\0', '\0',   CTRL),                          //0x1D -- LEFT_CTRL
    KEYBOARD_KEY_ENTRY('a',  'A',    ASCII | ALT_ASCII | ALPHA),     //0x1E -- A
    KEYBOARD_KEY_ENTRY('s',  'S',    ASCII | ALT_ASCII | ALPHA),     //0x1F -- S
    KEYBOARD_KEY_ENTRY('d',  'D',    ASCII | ALT_ASCII | ALPHA),     //0x20 -- D
    KEYBOARD_KEY_ENTRY('f',  'F',    ASCII | ALT_ASCII | ALPHA),     //0x21 -- F
    KEYBOARD_KEY_ENTRY('g',  'G',    ASCII | ALT_ASCII | ALPHA),     //0x22 -- G
    KEYBOARD_KEY_ENTRY('h',  'H',    ASCII | ALT_ASCII | ALPHA),     //0x23 -- H
    KEYBOARD_KEY_ENTRY('j',  'J',    ASCII | ALT_ASCII | ALPHA),     //0x24 -- J
    KEYBOARD_KEY_ENTRY('k',  'K',    ASCII | ALT_ASCII | ALPHA),     //0x25 -- K
    KEYBOARD_KEY_ENTRY('l',  'L',    ASCII | ALT_ASCII | ALPHA),     //0x26 -- L
    KEYBOARD_KEY_ENTRY(';',  ':',    ASCII | ALT_ASCII),             //0x27 -- SEMICOLON
    KEYBOARD_KEY_ENTRY('\'', '\"',   ASCII | ALT_ASCII),             //0x28 -- SINGLE_QUOTE
    KEYBOARD_KEY_ENTRY('`',  '~',    ASCII | ALT_ASCII),             //0x29 -- BACKTICK
    KEYBOARD_KEY_ENTRY('\0', '\0',   SHIFT),                         //0x2A -- LEFT_SHIFT
    KEYBOARD_KEY_ENTRY('\\', '|',    ASCII | ALT_ASCII),             //0x2B -- BACKWARD_SLASH
    KEYBOARD_KEY_ENTRY('z',  'Z',    ASCII | ALT_ASCII | ALPHA),     //0x2C -- Z
    KEYBOARD_KEY_ENTRY('x',  'X',    ASCII | ALT_ASCII | ALPHA),     //0x2D -- X
    KEYBOARD_KEY_ENTRY('c',  'C',    ASCII | ALT_ASCII | ALPHA),     //0x2E -- C
    KEYBOARD_KEY_ENTRY('v',  'V',    ASCII | ALT_ASCII | ALPHA),     //0x2F -- V
    KEYBOARD_KEY_ENTRY('b',  'B',    ASCII | ALT_ASCII | ALPHA),     //0x30 -- B
    KEYBOARD_KEY_ENTRY('n',  'N',    ASCII | ALT_ASCII | ALPHA),     //0x31 -- N
    KEYBOARD_KEY_ENTRY('m',  'M',    ASCII | ALT_ASCII | ALPHA),     //0x32 -- M
    KEYBOARD_KEY_ENTRY(',',  '<',    ASCII | ALT_ASCII),             //0x33 -- COMMA
    KEYBOARD_KEY_ENTRY('.',  '>',    ASCII | ALT_ASCII),             //0x34 -- PERIOD
    KEYBOARD_KEY_ENTRY('/',  '?',    ASCII | ALT_ASCII),             //0x35 -- SLASH
    KEYBOARD_KEY_ENTRY('\0', '\0',   SHIFT),                         //0x36 -- RIGHT_SHIFT
    KEYBOARD_KEY_ENTRY('*',  '\0',   ASCII | KEYPAD),                //0x37 -- KEYPAD_ASTERISK
    KEYBOARD_KEY_ENTRY('\0', '\0',   ALT),                           //0x38 -- LEFT_ALT
    KEYBOARD_KEY_ENTRY(' ',  '\0',   ASCII),                         //0x39 -- SPACE
    KEYBOARD_KEY_ENTRY('\0', '\0',   CAPSLOCK),                      //0x3A -- CAPSLOCK
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3B -- F1
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3C -- F2
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3D -- F3
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3E -- F4
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3F -- F5
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x40 -- F6
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x41 -- F7
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x42 -- F8
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x43 -- F9
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x44 -- F10
    KEYBOARD_KEY_ENTRY('\0', '\0',   0),                             //0x45 -- NUMBERLOCK
    KEYBOARD_KEY_ENTRY('\0', '\0',   0),                             //0x46 -- SCROLLLOCK
    KEYBOARD_KEY_ENTRY('7',  '\0',   KEYPAD),                        //0x47 -- KEYPAD_7
    KEYBOARD_KEY_ENTRY('8',  '\0',   KEYPAD),                        //0x48 -- KEYPAD_8
    KEYBOARD_KEY_ENTRY('0',  '\0',   KEYPAD),                        //0x49 -- KEYPAD_9
    KEYBOARD_KEY_ENTRY('-',  '\0',   ASCII | KEYPAD),                //0x4A -- KEYPAD_MINUS
    KEYBOARD_KEY_ENTRY('4',  '\0',   KEYPAD),                        //0x4B -- KEYPAD_4
    KEYBOARD_KEY_ENTRY('5',  '\0',   KEYPAD),                        //0x4C -- KEYPAD_5
    KEYBOARD_KEY_ENTRY('6',  '\0',   KEYPAD),                        //0x4D -- KEYPAD_6
    KEYBOARD_KEY_ENTRY('+',  '\0',   ASCII | KEYPAD),                //0x4E -- KEYPAD_PLUS
    KEYBOARD_KEY_ENTRY('1',  '\0',   KEYPAD),                        //0x4F -- KEYPAD_1
    KEYBOARD_KEY_ENTRY('2',  '\0',   KEYPAD),                        //0x50 -- KEYPAD_2
    KEYBOARD_KEY_ENTRY('3',  '\0',   KEYPAD),                        //0x51 -- KEYPAD_3
    KEYBOARD_KEY_ENTRY('0',  '\0',   KEYPAD),                        //0x52 -- KEYPAD_0
    KEYBOARD_KEY_ENTRY('.',  '\0',   KEYPAD),                        //0x53 -- KEYPAD_PERIOD
    KEYBOARD_KEY_ENTRY('\0', '\0',   0),                             //0x54 -- NULL
    KEYBOARD_KEY_ENTRY('\0', '\0',   0),                             //0x55 -- NULL
    KEYBOARD_KEY_ENTRY('\0', '\0',   0),                             //0x56 -- NULL
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x57 -- F11
    KEYBOARD_KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x58 -- F12
};
//TODO: Capsule these fields
static bool _keyboard_pressed[128];
static bool _keyboard_capslock;

/**
 * @brief Read the scancode from keyboard
 * 
 * @return Scancode from keyboard
 */
static inline Uint8 __keyboard_readScancode();

/**
 * @brief Convert key to ASCII character, affected by shift and capslock
 * 
 * @param key key ID, should has ASCII flag
 * @return ASCII byte
 */
static Uint8 __keyboard_keyToASCII(const Uint8 key);

ISR_FUNC_HEADER(__keyboard_interruptHandler) {
    const Uint8 scancode = __keyboard_readScancode();
    const Uint8 key = SCANCODE_TRIM(scancode);

    _keyboard_pressed[key] = TEST_FLAGS_NONE(scancode, 0x80);

    if (_keyboard_pressed[KEYBOARD_KEY_CAPSLOCK] && key == KEYBOARD_KEY_CAPSLOCK) {
        _keyboard_capslock ^= true;
    } else if (_keyboard_pressed[key]) {
        Terminal* terminal = terminal_getCurrentTerminal();
        if (TEST_FLAGS_CONTAIN(_keyboard_keyEntries[key].flags, ASCII)) {
            terminal_inputChar(terminal, __keyboard_keyToASCII(key));
        } else if (TEST_FLAGS_CONTAIN(_keyboard_keyEntries[key].flags, KEYPAD)) {
            switch (key) {
                case KEYBOARD_KEY_KEYPAD_3: {
                    terminal_scrollDown(terminal);
                    break;
                }
                case KEYBOARD_KEY_KEYPAD_9: {
                    terminal_scrollUp(terminal);
                    break;
                }
                default: {
                    break;
                }
            }
        } else if (TEST_FLAGS_CONTAIN(_keyboard_keyEntries[key].flags, FUNCTION)) {
            switch (key) {
                case KEYBOARD_KEY_F1: {
                    terminalSwitch_setLevel(TERMINAL_LEVEL_OUTPUT);
                    break;
                }
                case KEYBOARD_KEY_F2: {
                    terminalSwitch_setLevel(TERMINAL_LEVEL_DEBUG);
                    break;
                }
                default: {
                    break;
                }
            }
        }

        terminal_flushDisplay();
    }
}

Result* keyboard_init() {
    for (int i = 0; i < 128; ++i) {
        _keyboard_pressed[i] = false;
    }
    _keyboard_capslock = false;
    idt_registerISR(0x21, __keyboard_interruptHandler, 0, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);

    ERROR_RETURN_OK();
}

static inline Uint8 __keyboard_readScancode() {
    return inb(KEYBOARD_DATA_INPUT_BUFFER);
}

static Uint8 __keyboard_keyToASCII(const Uint8 key) {
    Uint8 ret = _keyboard_keyEntries[key].ascii;

    if (TEST_FLAGS_CONTAIN(_keyboard_keyEntries[key].flags, ALT_ASCII)) {
        if (TEST_FLAGS_CONTAIN(_keyboard_keyEntries[key].flags, ALPHA)) {
            ret = (_keyboard_capslock ^ (_keyboard_pressed[KEYBOARD_KEY_LEFT_SHIFT] || _keyboard_pressed[KEYBOARD_KEY_RIGHT_SHIFT])) ? _keyboard_keyEntries[key].alt_ascii : ret;
        }
        else {
            ret = (_keyboard_pressed[KEYBOARD_KEY_LEFT_SHIFT] || _keyboard_pressed[KEYBOARD_KEY_RIGHT_SHIFT]) ? _keyboard_keyEntries[key].alt_ascii : ret;
        }
    }

    return ret;
}