#if !defined(__PORTS_PIT_H)
#define __PORTS_PIT_H

#include<kit/bit.h>

//PIT(Programmable Interrupt Timer)

//PIT counter and control ports
#define PIT_COUTER_0                        0x0040
#define PIT_COUTER_1                        0x0041
#define PIT_COUTER_2                        0x0042
#define PIT_COUNTER_CONTROL                 0x0043

//PIT counter control words
#define PIT_BINARY_BCD_SWITCH               FLAG8(0)

#define PIT_COUNTER_MODE_0                  VAL_LEFT_SHIFT(0, 1)    //Interrupt on terminal count
#define PIT_COUNTER_MODE_1                  VAL_LEFT_SHIFT(1, 1)    //Hardware re-triggerable one-shot
#define PIT_COUNTER_MODE_2                  VAL_LEFT_SHIFT(2, 1)    //Rate generator
#define PIT_COUNTER_MODE_3                  VAL_LEFT_SHIFT(3, 1)    //Square wave generator
#define PIT_COUNTER_MODE_4                  VAL_LEFT_SHIFT(4, 1)    //Software triggered strobe
#define PIT_COUNTER_MODE_5                  VAL_LEFT_SHIFT(5, 1)    //Hardware triggered strobe

#define PIT_COUNTER_LATCH                   VAL_LEFT_SHIFT(0, 4)
#define PIT_COUNTER_BITS_MASK_0_7           VAL_LEFT_SHIFT(1, 4)
#define PIT_COUNTER_BITS_MASK_8_15          VAL_LEFT_SHIFT(2, 4)
#define PIT_COUNTER_BITS_MASK_LOW_HIGH_SEP  VAL_LEFT_SHIFT(3, 4)

#define PIT_COUNTER_SELECT_0                VAL_LEFT_SHIFT(0, 6)
#define PIT_COUNTER_SELECT_1                VAL_LEFT_SHIFT(1, 6)
#define PIT_COUNTER_SELECT_2                VAL_LEFT_SHIFT(2, 6)

#endif // __PORTS_PIT_H
