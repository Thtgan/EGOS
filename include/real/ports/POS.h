#if !defined(__REAL_PORTS_POS_H)
#define __REAL_PORTS_POS_H

#include<kit/bit.h>

//POS(Programmable Option Select)

//PS/2 system control ports
#define POS_PS2_SYSTEM_CONTROL_A    0x0092

//PS/2 system control port A flags
#define POS_ALTERNATE_CPU_RESET     FLAG8(0)     //Pulse alternate reset pin (System reset or write)
#define POS_A20_ACTIVE              FLAG8(1)
#define POS_CMOS_LOCK               FLAG8(3)     //RTC/CMOS (un)locked
#define POS_WATCHDOG_TIMEOUT        FLAG8(4)     //Wathcdog timeout (not) occured
#define POS_ACTIVITY_LIGHT_ON       FLAG8(6)     //Turn activity light on/off (6-7 bit all 0)

#endif // __REAL_PORTS_POS_H
