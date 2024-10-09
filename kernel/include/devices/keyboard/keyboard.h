#if !defined(__DEVICES_KEYBOARD_KEYBOARD_H)
#define __DEVICES_KEYBOARD_KEYBOARD_H

typedef struct KeyboardKeyEntry KeyboardKeyEntry;

#include<kit/bit.h>
#include<kit/types.h>

typedef struct KeyboardKeyEntry {
    Uint8 ascii, alt_ascii;
    Uint16 flags;
} KeyboardKeyEntry;

#define KEYBOARD_KEY_ESC                    0x01
#define KEYBOARD_KEY_NUMBER_1               0x02
#define KEYBOARD_KEY_NUMBER_2               0x03
#define KEYBOARD_KEY_NUMBER_3               0x04
#define KEYBOARD_KEY_NUMBER_4               0x05
#define KEYBOARD_KEY_NUMBER_5               0x06
#define KEYBOARD_KEY_NUMBER_6               0x07
#define KEYBOARD_KEY_NUMBER_7               0x08
#define KEYBOARD_KEY_NUMBER_8               0x09
#define KEYBOARD_KEY_NUMBER_9               0x0A
#define KEYBOARD_KEY_NUMBER_0               0x0B
#define KEYBOARD_KEY_HYPHEN                 0x0C
#define KEYBOARD_KEY_EQUAL                  0x0D
#define KEYBOARD_KEY_BACKSPACE              0x0E
#define KEYBOARD_KEY_TAB                    0x0F
#define KEYBOARD_KEY_Q                      0x10
#define KEYBOARD_KEY_W                      0x11
#define KEYBOARD_KEY_E                      0x12
#define KEYBOARD_KEY_R                      0x13
#define KEYBOARD_KEY_T                      0x14
#define KEYBOARD_KEY_Y                      0x15
#define KEYBOARD_KEY_U                      0x16
#define KEYBOARD_KEY_I                      0x17
#define KEYBOARD_KEY_O                      0x18
#define KEYBOARD_KEY_P                      0x19
#define KEYBOARD_KEY_LEFT_SQUARE_BARCKET    0x1A
#define KEYBOARD_KEY_RIGHT_SQUARE_BARCKET   0x1B
#define KEYBOARD_KEY_ENTER                  0x1C
#define KEYBOARD_KEY_LEFT_CTRL              0x1D
#define KEYBOARD_KEY_A                      0x1E
#define KEYBOARD_KEY_S                      0x1F
#define KEYBOARD_KEY_D                      0x20
#define KEYBOARD_KEY_F                      0x21
#define KEYBOARD_KEY_G                      0x22
#define KEYBOARD_KEY_H                      0x23
#define KEYBOARD_KEY_J                      0x24
#define KEYBOARD_KEY_K                      0x25
#define KEYBOARD_KEY_L                      0x26
#define KEYBOARD_KEY_SEMICOLON              0x27
#define KEYBOARD_KEY_SINGLE_QUOTE           0x28
#define KEYBOARD_KEY_BACKTICK               0x29
#define KEYBOARD_KEY_LEFT_SHIFT             0x2A
#define KEYBOARD_KEY_BACKWARD_SLASH         0x2B
#define KEYBOARD_KEY_Z                      0x2C
#define KEYBOARD_KEY_X                      0x2D
#define KEYBOARD_KEY_C                      0x2E
#define KEYBOARD_KEY_V                      0x2F
#define KEYBOARD_KEY_B                      0x30
#define KEYBOARD_KEY_N                      0x31
#define KEYBOARD_KEY_M                      0x32
#define KEYBOARD_KEY_COMMA                  0x33
#define KEYBOARD_KEY_PERIOD                 0x34
#define KEYBOARD_KEY_SLASH                  0x35
#define KEYBOARD_KEY_RIGHT_SHIFT            0x36
#define KEYBOARD_KEY_KEYPAD_ASTERISK        0x37
#define KEYBOARD_KEY_LEFT_ALT               0x38
#define KEYBOARD_KEY_SPACE                  0x39
#define KEYBOARD_KEY_CAPSLOCK               0x3A
#define KEYBOARD_KEY_F1                     0x3B
#define KEYBOARD_KEY_F2                     0x3C
#define KEYBOARD_KEY_F3                     0x3D
#define KEYBOARD_KEY_F4                     0x3E
#define KEYBOARD_KEY_F5                     0x3F
#define KEYBOARD_KEY_F6                     0x40
#define KEYBOARD_KEY_F7                     0x41
#define KEYBOARD_KEY_F8                     0x42
#define KEYBOARD_KEY_F9                     0x43
#define KEYBOARD_KEY_F10                    0x44
#define KEYBOARD_KEY_NUMBERLOCK             0x45
#define KEYBOARD_KEY_SCROLLLOCK             0x46
#define KEYBOARD_KEY_KEYPAD_7               0x47 //HOME
#define KEYBOARD_KEY_KEYPAD_8               0x48 //UP ARROW
#define KEYBOARD_KEY_KEYPAD_9               0x49 //PAGE UP
#define KEYBOARD_KEY_KEYPAD_MINUS           0x4A 
#define KEYBOARD_KEY_KEYPAD_4               0x4B //LEFT ARROW
#define KEYBOARD_KEY_KEYPAD_5               0x4C
#define KEYBOARD_KEY_KEYPAD_6               0x4D //RIGHT ARROW
#define KEYBOARD_KEY_KEYPAD_PLUS            0x4E
#define KEYBOARD_KEY_KEYPAD_1               0x4F //END
#define KEYBOARD_KEY_KEYPAD_2               0x50 //DOWN AWWOW
#define KEYBOARD_KEY_KEYPAD_3               0x51 //PAGE DOWN
#define KEYBOARD_KEY_KEYPAD_0               0x52 //INSERT
#define KEYBOARD_KEY_KEYPAD_PERIOD          0x53 //DELETE
#define KEYBOARD_KEY_F11                    0x57
#define KEYBOARD_KEY_F12                    0x58

#define SCANCODE_TRIM(__SCANCODE)                       TRIM_VAL_SIMPLE(__SCANCODE, 8, 7)
#define SCANCODE_PRESS(__SCANCODE)                      TRIM_VAL_SIMPLE(__SCANCODE, 8, 7)
#define SCANCODE_RELEASE(__SCANCODE)                    FILL_VAL(__SCANCODE, 0x80)

#define KEYBOARD_KEY_ENTRY(__ASCII, __SHIFT_ASCII, __FLAGS)      (KeyboardKeyEntry) {__ASCII, __SHIFT_ASCII, __FLAGS}
#define ASCII                                           FLAG16(0)
#define ALT_ASCII                                       FLAG16(1)
#define ALPHA                                           FLAG16(2)
#define CONTROL                                         FLAG16(3)
#define KEYPAD                                          FLAG16(4)
#define FUNCTION                                        FLAG16(5)
#define CTRL                                            FLAG16(6)
#define SHIFT                                           FLAG16(7)
#define ALT                                             FLAG16(8)
#define CAPSLOCK                                        FLAG16(9)
#define ENTER                                           FLAG16(10)

/**
 * @brief Initialize the keyboard
 * 
 * @return Result Result of the operation
 */
Result keyboard_init();

#endif // __DEVICES_KEYBOARD_KEYBOARD_H
