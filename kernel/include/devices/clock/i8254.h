#if !defined(__I8254_H)
#define __I8254_H

#include<devices/clock/clockSource.h>
#include<kit/types.h>

#define CLOCK_SOURCE_I8254_DEFAULT_FREQUENCY    100

Result i8254_initClockSource(ClockSource* clockSource);

#endif // __I8254_H
