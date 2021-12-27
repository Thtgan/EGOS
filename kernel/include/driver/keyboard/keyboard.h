#if !defined(__KEYBOARD_H)
#define __KEYBOARD_H

#include<kit/bit.h>

typedef struct {
    uint8_t ascii, alt_ascii;
    uint16_t flags;
} KeyEntry;

#define KEY_ESC                     0x01
#define KEY_NUMBER_1                0x02
#define KEY_NUMBER_2                0x03
#define KEY_NUMBER_3                0x04
#define KEY_NUMBER_4                0x05
#define KEY_NUMBER_5                0x06
#define KEY_NUMBER_6                0x07
#define KEY_NUMBER_7                0x08
#define KEY_NUMBER_8                0x09
#define KEY_NUMBER_9                0x0A
#define KEY_NUMBER_0                0x0B
#define KEY_HYPHEN                  0x0C
#define KEY_EQUAL                   0x0D
#define KEY_BACKSPACE               0x0E
#define KEY_TAB                     0x0F
#define KEY_Q                       0x10
#define KEY_W                       0x11
#define KEY_E                       0x12
#define KEY_R                       0x13
#define KEY_T                       0x14
#define KEY_Y                       0x15
#define KEY_U                       0x16
#define KEY_I                       0x17
#define KEY_O                       0x18
#define KEY_P                       0x19
#define KEY_LEFT_SQUARE_BARCKET     0x1A
#define KEY_RIGHT_SQUARE_BARCKET    0x1B
#define KEY_ENTER                   0x1C
#define KEY_LEFT_CTRL               0x1D
#define KEY_A                       0x1E
#define KEY_S                       0x1F
#define KEY_D                       0x20
#define KEY_F                       0x21
#define KEY_G                       0x22
#define KEY_H                       0x23
#define KEY_J                       0x24
#define KEY_K                       0x25
#define KEY_L                       0x26
#define KEY_SEMICOLON               0x27
#define KEY_SINGLE_QUOTE            0x28
#define KEY_BACKTICK                0x29
#define KEY_LEFT_SHIFT              0x2A
#define KEY_BACKWARD_SLASH          0x2B
#define KEY_Z                       0x2C
#define KEY_X                       0x2D
#define KEY_C                       0x2E
#define KEY_V                       0x2F
#define KEY_B                       0x30
#define KEY_N                       0x31
#define KEY_M                       0x32
#define KEY_COMMA                   0x33
#define KEY_PERIOD                  0x34
#define KEY_SLASH                   0x35
#define KEY_RIGHT_SHIFT             0x36
#define KEY_KEYPAD_ASTERISK         0x37
#define KEY_LEFT_ALT                0x38
#define KEY_SPACE                   0x39
#define KEY_CAPSLOCK                0x3A
#define KEY_F1                      0x3B
#define KEY_F2                      0x3C
#define KEY_F3                      0x3D
#define KEY_F4                      0x3E
#define KEY_F5                      0x3F
#define KEY_F6                      0x40
#define KEY_F7                      0x41
#define KEY_F8                      0x42
#define KEY_F9                      0x43
#define KEY_F10                     0x44
#define KEY_NUMBERLOCK              0x45
#define KEY_SCROLLLOCK              0x46
#define KEY_KEYPAD_7                0x47 //HOME
#define KEY_KEYPAD_8                0x48 //UP ARROW
#define KEY_KEYPAD_9                0x49 //PAGE UP
#define KEY_KEYPAD_MINUS            0x4A 
#define KEY_KEYPAD_4                0x4B //LEFT ARROW
#define KEY_KEYPAD_5                0x4C
#define KEY_KEYPAD_6                0x4D //RIGHT ARROW
#define KEY_KEYPAD_PLUS             0x4E
#define KEY_KEYPAD_1                0x4F //END
#define KEY_KEYPAD_2                0x50 //DOWN AWWOW
#define KEY_KEYPAD_3                0x51 //PAGE DOWN
#define KEY_KEYPAD_0                0x52 //INSERT
#define KEY_KEYPAD_PERIOD           0x53 //DELETE
#define KEY_F11                     0x57
#define KEY_F12                     0x58

#define SCANCODE_TRIM(__SCANCODE)                       TRIM_VAL_SIMPLE(__SCANCODE, 8, 7)
#define SCANCODE_PRESS(__SCANCODE)                      TRIM_VAL_SIMPLE(__SCANCODE, 8, 7)
#define SCANCODE_RELEASE(__SCANCODE)                    FILL_VAL(__SCANCODE, 0x80)

#define KEY_ENTRY(__ASCII, __SHIFT_ASCII, __FLAGS)   (KeyEntry) {__ASCII, __SHIFT_ASCII, __FLAGS}
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
 */
void keyboardInit();

#endif // __KEYBOARD_H
