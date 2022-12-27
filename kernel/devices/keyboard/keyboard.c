#include<devices/keyboard/keyboard.h>

#include<devices/vga/textmode.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<real/ports/keyboard.h>
#include<real/simpleAsmLines.h>

__attribute__((aligned(4)))
static KeyEntry _keyEntries[128] = {
    KEY_ENTRY('\0', '\0',   0),                            //0x00 -- NULL
    KEY_ENTRY('\0', '\0',   0),                            //0x01 -- ESC
    KEY_ENTRY('1',  '!',    ASCII | ALT_ASCII),            //0x02 -- 1
    KEY_ENTRY('2',  '@',    ASCII | ALT_ASCII),            //0x03 -- 2
    KEY_ENTRY('3',  '#',    ASCII | ALT_ASCII),            //0x04 -- 3
    KEY_ENTRY('4',  '$',    ASCII | ALT_ASCII),            //0x05 -- 4
    KEY_ENTRY('5',  '%',    ASCII | ALT_ASCII),            //0x06 -- 5
    KEY_ENTRY('6',  '^',    ASCII | ALT_ASCII),            //0x07 -- 6
    KEY_ENTRY('7',  '&',    ASCII | ALT_ASCII),            //0x08 -- 7
    KEY_ENTRY('8',  '*',    ASCII | ALT_ASCII),            //0x09 -- 8
    KEY_ENTRY('9',  '(',    ASCII | ALT_ASCII),            //0x0A -- 9
    KEY_ENTRY('0',  ')',    ASCII | ALT_ASCII),             //0x0B -- 0
    KEY_ENTRY('-',  '_',    ASCII | ALT_ASCII),             //0x0C -- HYPHEN
    KEY_ENTRY('=',  '+',    ASCII | ALT_ASCII),             //0x0D -- EQUAL
    KEY_ENTRY('\b', '\0',   ASCII),                         //0x0E -- BACKSPACE
    KEY_ENTRY('\t', '\0',   ASCII),                         //0x0F -- TAB
    KEY_ENTRY('q',  'Q',    ASCII | ALT_ASCII | ALPHA),     //0x10 -- Q
    KEY_ENTRY('w',  'W',    ASCII | ALT_ASCII | ALPHA),     //0x11 -- W
    KEY_ENTRY('e',  'E',    ASCII | ALT_ASCII | ALPHA),     //0x12 -- E
    KEY_ENTRY('r',  'R',    ASCII | ALT_ASCII | ALPHA),     //0x13 -- R
    KEY_ENTRY('t',  'T',    ASCII | ALT_ASCII | ALPHA),     //0x14 -- T
    KEY_ENTRY('y',  'Y',    ASCII | ALT_ASCII | ALPHA),     //0x15 -- Y
    KEY_ENTRY('u',  'U',    ASCII | ALT_ASCII | ALPHA),     //0x16 -- U
    KEY_ENTRY('i',  'I',    ASCII | ALT_ASCII | ALPHA),     //0x17 -- I
    KEY_ENTRY('o',  'O',    ASCII | ALT_ASCII | ALPHA),     //0x18 -- O
    KEY_ENTRY('p',  'P',    ASCII | ALT_ASCII | ALPHA),     //0x19 -- P
    KEY_ENTRY('[',  '{',    ASCII | ALT_ASCII),             //0x1A -- LEFT_SQUARE_BARCKET
    KEY_ENTRY(']',  '}',    ASCII | ALT_ASCII),             //0x1B -- LEFT_SQUARE_BARCKET
    KEY_ENTRY('\n', '\0',   ASCII | ENTER),                 //0x1C -- ENTER
    KEY_ENTRY('\0', '\0',   CTRL),                          //0x1D -- LEFT_CTRL
    KEY_ENTRY('a',  'A',    ASCII | ALT_ASCII | ALPHA),     //0x1E -- A
    KEY_ENTRY('s',  'S',    ASCII | ALT_ASCII | ALPHA),     //0x1F -- S
    KEY_ENTRY('d',  'D',    ASCII | ALT_ASCII | ALPHA),     //0x20 -- D
    KEY_ENTRY('f',  'F',    ASCII | ALT_ASCII | ALPHA),     //0x21 -- F
    KEY_ENTRY('g',  'G',    ASCII | ALT_ASCII | ALPHA),     //0x22 -- G
    KEY_ENTRY('h',  'H',    ASCII | ALT_ASCII | ALPHA),     //0x23 -- H
    KEY_ENTRY('j',  'J',    ASCII | ALT_ASCII | ALPHA),     //0x24 -- J
    KEY_ENTRY('k',  'K',    ASCII | ALT_ASCII | ALPHA),     //0x25 -- K
    KEY_ENTRY('l',  'L',    ASCII | ALT_ASCII | ALPHA),     //0x26 -- L
    KEY_ENTRY(';',  ':',    ASCII | ALT_ASCII),             //0x27 -- SEMICOLON
    KEY_ENTRY('\'', '\"',   ASCII | ALT_ASCII),             //0x28 -- SINGLE_QUOTE
    KEY_ENTRY('`',  '~',    ASCII | ALT_ASCII),             //0x29 -- BACKTICK
    KEY_ENTRY('\0', '\0',   SHIFT),                         //0x2A -- LEFT_SHIFT
    KEY_ENTRY('\\', '|',    ASCII | ALT_ASCII),             //0x2B -- BACKWARD_SLASH
    KEY_ENTRY('z',  'Z',    ASCII | ALT_ASCII | ALPHA),     //0x2C -- Z
    KEY_ENTRY('x',  'X',    ASCII | ALT_ASCII | ALPHA),     //0x2D -- X
    KEY_ENTRY('c',  'C',    ASCII | ALT_ASCII | ALPHA),     //0x2E -- C
    KEY_ENTRY('v',  'V',    ASCII | ALT_ASCII | ALPHA),     //0x2F -- V
    KEY_ENTRY('b',  'B',    ASCII | ALT_ASCII | ALPHA),     //0x30 -- B
    KEY_ENTRY('n',  'N',    ASCII | ALT_ASCII | ALPHA),     //0x31 -- N
    KEY_ENTRY('m',  'M',    ASCII | ALT_ASCII | ALPHA),     //0x32 -- M
    KEY_ENTRY(',',  '<',    ASCII | ALT_ASCII),             //0x33 -- COMMA
    KEY_ENTRY('.',  '>',    ASCII | ALT_ASCII),             //0x34 -- PERIOD
    KEY_ENTRY('/',  '?',    ASCII | ALT_ASCII),             //0x35 -- SLASH
    KEY_ENTRY('\0', '\0',   SHIFT),                         //0x36 -- RIGHT_SHIFT
    KEY_ENTRY('*',  '\0',   ASCII | KEYPAD),                //0x37 -- KEYPAD_ASTERISK
    KEY_ENTRY('\0', '\0',   ALT),                           //0x38 -- LEFT_ALT
    KEY_ENTRY(' ',  '\0',   ASCII),                         //0x39 -- SPACE
    KEY_ENTRY('\0', '\0',   CAPSLOCK),                      //0x3A -- CAPSLOCK
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3B -- F1
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3C -- F2
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3D -- F3
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3E -- F4
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x3F -- F5
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x40 -- F6
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x41 -- F7
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x42 -- F8
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x43 -- F9
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x44 -- F10
    KEY_ENTRY('\0', '\0',   0),                             //0x45 -- NUMBERLOCK
    KEY_ENTRY('\0', '\0',   0),                             //0x46 -- SCROLLLOCK
    KEY_ENTRY('7',  '\0',   KEYPAD),                        //0x47 -- KEYPAD_7
    KEY_ENTRY('8',  '\0',   KEYPAD),                        //0x48 -- KEYPAD_8
    KEY_ENTRY('0',  '\0',   KEYPAD),                        //0x49 -- KEYPAD_9
    KEY_ENTRY('-',  '\0',   ASCII | KEYPAD),                //0x4A -- KEYPAD_MINUS
    KEY_ENTRY('4',  '\0',   KEYPAD),                        //0x4B -- KEYPAD_4
    KEY_ENTRY('5',  '\0',   KEYPAD),                        //0x4C -- KEYPAD_5
    KEY_ENTRY('6',  '\0',   KEYPAD),                        //0x4D -- KEYPAD_6
    KEY_ENTRY('+',  '\0',   ASCII | KEYPAD),                //0x4E -- KEYPAD_PLUS
    KEY_ENTRY('1',  '\0',   KEYPAD),                        //0x4F -- KEYPAD_1
    KEY_ENTRY('2',  '\0',   KEYPAD),                        //0x50 -- KEYPAD_2
    KEY_ENTRY('3',  '\0',   KEYPAD),                        //0x51 -- KEYPAD_3
    KEY_ENTRY('0',  '\0',   KEYPAD),                        //0x52 -- KEYPAD_0
    KEY_ENTRY('.',  '\0',   KEYPAD),                        //0x53 -- KEYPAD_PERIOD
    KEY_ENTRY('\0', '\0',   0),                             //0x54 -- NULL
    KEY_ENTRY('\0', '\0',   0),                             //0x55 -- NULL
    KEY_ENTRY('\0', '\0',   0),                             //0x56 -- NULL
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x57 -- F11
    KEY_ENTRY('\0', '\0',   FUNCTION),                      //0x58 -- F12
};

static bool _pressed[128];
static bool _capslock;

/**
 * @brief Read the scancode from keyboard
 * 
 * @return Scancode from keyboard
 */
static inline uint8_t __readScancode();

/**
 * @brief Convert key to ASCII character, affected by shift and capslock
 * 
 * @param key key ID, should has ASCII flag
 * @return ASCII byte
 */
static uint8_t __toASCII(const uint8_t key);

ISR_FUNC_HEADER(__keyboardInterrupt) {
    const uint8_t scancode = __readScancode();
    const uint8_t key = SCANCODE_TRIM(scancode);

    _pressed[key] = TEST_FLAGS_NONE(scancode, 0x80);

    if (_pressed[KEY_CAPSLOCK] && key == KEY_CAPSLOCK) {
        _capslock ^= true;
    }
    else if (_pressed[key]) {
        if (TEST_FLAGS_CONTAIN(_keyEntries[key].flags, ASCII)) {
            vgaPutchar(__toASCII(key));
        } else if (TEST_FLAGS_CONTAIN(_keyEntries[key].flags, KEYPAD)) {
            const TextModeInfo* tmInfo = getTextModeInfo();
            int row = vgaGetCursorRowIndex(), col = vgaGetCursorColIndex();
            switch (key)
            {
            case KEY_KEYPAD_1:
                col = TEXT_MODE_WIDTH - 1;
                break;
            case KEY_KEYPAD_2:
                ++row;
                if (row >= TEXT_MODE_HEIGHT) {
                    col = 0;
                }
                break;
            case KEY_KEYPAD_3:
                vgaScrollDown();
                break;
            case KEY_KEYPAD_4:
                --col;
                if (col < 0) {
                    --row, col = TEXT_MODE_WIDTH - 1;
                }
                break;
            case KEY_KEYPAD_5:
                break;
            case KEY_KEYPAD_6:
                ++col;
                if (col >= TEXT_MODE_WIDTH) {
                    ++row, col = 0;
                }
                break;
            case KEY_KEYPAD_7:
                col = 0;
                break;
            case KEY_KEYPAD_8:
                --row;
                break;
            case KEY_KEYPAD_9:
                vgaScrollUp();
                break;
            default:
                break;
            }
            vgaSetCursorPosition(row, col);
        }
    }

    EOI();
}

/**
 * @brief Initialize the keyboard
 */
void initKeyboard() {
    for (int i = 0; i < 128; ++i) {
        _pressed[i] = false;
    }
    _capslock = false;
    registerISR(0x21, __keyboardInterrupt, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
}

static inline uint8_t __readScancode() {
    return inb(KEYBOARD_DATA_INPUT_BUFFER);
}

static uint8_t __toASCII(const uint8_t key) {
    uint8_t ret = _keyEntries[key].ascii;

    if (TEST_FLAGS_CONTAIN(_keyEntries[key].flags, ALT_ASCII)) {
        if (TEST_FLAGS_CONTAIN(_keyEntries[key].flags, ALPHA)) {
            ret = (_capslock ^ (_pressed[KEY_LEFT_SHIFT] || _pressed[KEY_RIGHT_SHIFT])) ? _keyEntries[key].alt_ascii : ret;
        }
        else {
            ret = (_pressed[KEY_LEFT_SHIFT] || _pressed[KEY_RIGHT_SHIFT]) ? _keyEntries[key].alt_ascii : ret;
        }
    }

    return ret;
}