#include<devices/clock/clockSource.h>

#include<devices/clock/CMOS.h>
#include<devices/clock/CPUclock.h>
#include<devices/clock/i8254.h>
#include<kit/types.h>

static ClockSource _clockSources[CLOCK_SOURCE_TYPE_NUM];

void initClockSources() {
    initClockSourceCMOS(&_clockSources[CLOCK_SOURCE_TYPE_CMOS]);
    initClockSourceI8254(&_clockSources[CLOCK_SOURCE_TYPE_I8254]);
    initClockSourceCPU(&_clockSources[CLOCK_SOURCE_TYPE_CPU]);
}

ClockSource* getClockSource(ClockSourceType type) {
    if (type >= CLOCK_SOURCE_TYPE_NUM) {
        return NULL;
    }

    return &_clockSources[type];
}
