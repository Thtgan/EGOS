#if !defined(_PORTS_KEYBOARD_H)
#define _PORTS_KEYBOARD_H

#include<kit/bit.h>

//Keyboard buffers port
#define KEYBOARD_DATA_INPUT_BUFFER                                  0x0060
#define KEYBOARD_DATA_OUTPUT_BUFFER                                 0x0060

//Keyboard controller read status port
#define KEYBOARD_CONTROLLER_READ_STATUS                             0x0064

#define KEYBOARD_CONTROLLER_ISA_STATUS_OUTPUT_BUFFER_FULL           FLAG8(0)
#define KEYBOARD_CONTROLLER_ISA_STATUS_INPUT_BUFFER_FULL            FLAG8(1)
#define KEYBOARD_CONTROLLER_ISA_STATUS_SYSTEM_FLAG                  FLAG8(2)    //0 = power up or reset, 1 = self-test ok
#define KEYBOARD_CONTROLLER_ISA_STATUS_INPUT_REGISTER_DATA_TYPE     FLAG8(3)    //0 = data, 1 = command
#define KEYBOARD_CONTROLLER_ISA_STATUS_KEYBOARD_INHIBIT             FLAG8(4)    //0 = inhibited
#define KEYBOARD_CONTROLLER_ISA_STATUS_TRANSMIT_TIMEOUT             FLAG8(5)
#define KEYBOARD_CONTROLLER_ISA_STATUS_RECEIVE_TIMEOUT              FLAG8(6)
#define KEYBOARD_CONTROLLER_ISA_STATUS_TRANSMISSION_PARITY_ERROR    FLAG8(7)

//Keyboard controller read status port
#define KEYBOARD_CONTROLLER_INPUT_BUFFER                            0x0064

#define KEYBOARD_CONTROLLER_DISABLE_KEYBOARD                        0xAD
#define KEYBOARD_CONTROLLER_ENABLE_KEYBOARD                         0xAE
#define KEYBOARD_CONTROLLER_READ_OUTPUT                             0xD0
#define KEYBOARD_CONTROLLER_WRITE_OUTPUT                            0xD1
#define KEYBOARD_CONTROLLER_OUTPUT_FLAG_A20_CONTROL                 FLAG8(1)

#endif // _PORTS_KEYBOARD_H
