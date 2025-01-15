#if !defined(__DEVICES_CLOCK_CLOCKSOURCE_H)
#define __DEVICES_CLOCK_CLOCKSOURCE_H

typedef enum ClockSourceType {
    CLOCK_SOURCE_TYPE_CMOS,
    CLOCK_SOURCE_TYPE_I8254,    //TODO: Add support for HPET
    CLOCK_SOURCE_TYPE_CPU,
    CLOCK_SOURCE_TYPE_NUM
} ClockSourceType;

typedef struct ClockSource ClockSource;

#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<multitask/locks/spinlock.h>
#include<time/time.h>

typedef struct ClockSource {
    ClockSourceType type;
    Uint64          tick;
    Uint64          hz;
    Uint64          tickConvertMultiplier;
    Spinlock        lock;   //TODO: Sequence Lock?
    Flags8          flags;
#define CLOCK_SOURCE_FLAGS_PRESENT      FLAG8(0)
#define CLOCK_SOURCE_FLAGS_AUTO_UPDATE  FLAG8(1)

    Uint64          (*readTick)(ClockSource* this);
#define CLOCK_SOURCE_INVALID_TICK   -1
    Result          (*updateTick)(ClockSource* this);
    Result          (*start)(ClockSource* this);
    Result          (*stop)(ClockSource* this);
} ClockSource;

#define CLOCK_SOURCE_HZ_TO_TICK_CONVERT_MULTIPLER(__HZ)                         (((Uint64)TIME_UNIT_SECOND << 32) / (__HZ))
#define CLOCK_SOURCE_CONVERT_TICK_TO_TIME(__CLOCK_SOURCE, __TICK, __UNIT)       (((__TICK) * ((__CLOCK_SOURCE)->tickConvertMultiplier / (__UNIT))) >> 32)
#define CLOCK_SOURCE_GET_TICK_REMAIN(__CLOCK_SOURCE, __TIME, __TICK, __UNIT)    ((__TICK) - ((__TIME) * (__CLOCK_SOURCE)->hz) / (TIME_UNIT_SECOND / (__UNIT)))
#define CLOCK_SOURCE_CONVERT_TIME_TO_TICK(__CLOCK_SOURCE, __TIME, __UNIT)       ((__TIME) * (__UNIT) * (__CLOCK_SOURCE)->hz / TIME_UNIT_SECOND)

void clockSources_init();

ClockSource* clockSource_getSource(ClockSourceType type);

static inline Uint64 rawClockSourceReadTick(ClockSource* clockSource) {
    return clockSource->readTick(clockSource);
}

static inline Result rawClockSourceUpdateTick(ClockSource* clockSource) {
    return clockSource->updateTick(clockSource);
}

static inline Result rawClockSourceStart(ClockSource* clockSource) {
    return clockSource->start(clockSource);
}

static inline Result rawClockSourceStop(ClockSource* clockSource) {
    return clockSource->stop(clockSource);
}

#endif // __DEVICES_CLOCK_CLOCKSOURCE_H
