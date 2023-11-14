#if !defined(__PORTS_PIT_H)
#define __PORTS_PIT_H

#include<kit/bit.h>

//PIT(Programmable Interrupt Timer)

#define PIT_MAX_FREQUENCY                               1193182u

//PIT counter and control ports
#define PIT_CHANNEL_SELECT(__CHANNEL)                   (0x40 + __CHANNEL)
#define PIT_CONTROL                                     0x0043

//PIT counter control words
#define PIT_CONTROL_BINARY_BCD_SWITCH                   FLAG8(0)

//0 -- Interrupt on terminal count
//1 -- Hardware re-triggerable one-shot
//2 -- Rate generator
//3 -- Square wave generator
//4 -- Software triggered strobe
//5 -- Hardware triggered strobe
#define PIT_CONTROL_MODE_INTERRUPT_ON_TERMINAL_COUNT    VAL_LEFT_SHIFT(0, 1)
#define PIT_CONTROL_MODE_ONE_SHOT                       VAL_LEFT_SHIFT(1, 1)
#define PIT_CONTROL_MODE_RATE_GENERATOR                 VAL_LEFT_SHIFT(2, 1)
#define PIT_CONTROL_MODE_SQUARE_WAVE_GENERATOR          VAL_LEFT_SHIFT(3, 1)
#define PIT_CONTROL_MODE_SOFTWARE_TRIGGERED_STROBE      VAL_LEFT_SHIFT(4, 1)
#define PIT_CONTROL_MODE_HARDWARE_TRIGGERED_STROBE      VAL_LEFT_SHIFT(5, 1)

#define PIT_CONTROL_LATCH                               VAL_LEFT_SHIFT(0, 4)
#define PIT_CONTROL_BITS_MASK_0_7                       VAL_LEFT_SHIFT(1, 4)
#define PIT_CONTROL_BITS_MASK_8_15                      VAL_LEFT_SHIFT(2, 4)
#define PIT_CONTROL_BITS_MASK_LOW_HIGH_SEP              VAL_LEFT_SHIFT(3, 4)

#define PIT_CONTROL_CHANNEL_SELECT(__CHANNEL)           VAL_LEFT_SHIFT(__CHANNEL, 6)
#define PIT_CONTROL_READBACK                            VAL_LEFT_SHIFT(3, 6)

#define PIT_READBACK_NO_LATCH_STATUS                    FLAG8(4)
#define PIT_READBACK_NO_LATCH_COUNT                     FLAG8(5)
#define PIT_READBACK_CHANNEL(__CHANNEL)                 VAL_LEFT_SHIFT(__CHANNEL, 1)

#endif // __PORTS_PIT_H
