#include<devices/clock/clockSource.h>

#include<devices/clock/CMOS.h>
#include<devices/clock/CPUclock.h>
#include<devices/clock/i8254.h>
#include<kit/types.h>

static ClockSource _clockSources[CLOCK_SOURCE_TYPE_NUM];

void clockSources_init() {
    CMOS_initClockSource(&_clockSources[CLOCK_SOURCE_TYPE_CMOS]);
    i8254_initClockSource(&_clockSources[CLOCK_SOURCE_TYPE_I8254]);
    CPUclock_initClockSource(&_clockSources[CLOCK_SOURCE_TYPE_CPU]);
}

ClockSource* clockSource_getSource(ClockSourceType type) {
    if (type >= CLOCK_SOURCE_TYPE_NUM) {
        return NULL;
    }

    return &_clockSources[type];
}
