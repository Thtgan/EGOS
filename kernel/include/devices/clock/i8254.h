#if !defined(__DEVICES_CLOCK_I8254_H)
#define __DEVICES_CLOCK_I8254_H

#include<devices/clock/clockSource.h>
#include<kit/types.h>

#define CLOCK_SOURCE_I8254_DEFAULT_FREQUENCY    100

OldResult i8254_initClockSource(ClockSource* clockSource);

#endif // __DEVICES_CLOCK_I8254_H
