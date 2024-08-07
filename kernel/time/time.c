#include<time/time.h>

#include<devices/clock/clockSource.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<multitask/schedule.h>
#include<time/timer.h>

#define __TIME_DAYS_IN_ERA          146097ll
#define __TIME_DAYS_IN_4_YEARS      1461ll
#define __TIME_DAYS_IN_100_YEARS    36524ll
#define __TIME_DAYS_BEFORE_1970     719468ll

void time_convertRealTimeToTimestamp(RealTime* realTime, Timestamp* timestamp) {
    int year = realTime->year, month = realTime->month, day = realTime->day;

    if (month <= 2) {   //Make March first month of year
        --year;
        month += 9;
    } else {
        month -= 3;
    }
    --day;  //Month and day starts from 0;

    int era = (year >= 0 ? year : year - 399) / 400;    //400 years per era for rules of leap year
    int yearOfEra = year - era * 400;
    int dayOfYear = (153 * month + 2) / 5 + day;
    int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    Int64 timestampDay = era * __TIME_DAYS_IN_ERA + dayOfEra - __TIME_DAYS_BEFORE_1970;

    timestamp->second = ((timestampDay * 24 + realTime->hour) * 60 + realTime->minute) * 60 + realTime->second;
    timestamp->nanosecond = (realTime->millisecond * 1000 + realTime->microsecond) * 1000 + realTime->nanosecond;
}

void time_convertTimestampToRealTime(Timestamp* timestamp, RealTime* realTime) {
    realTime->nanosecond = timestamp->nanosecond % 1000;
    realTime->microsecond = (timestamp->nanosecond / 1000) % 1000;
    realTime->millisecond = (timestamp->nanosecond / 1000000) % 1000;

    Int64 second = timestamp->second;
    realTime->second = second % 60;
    second /= 60;
    realTime->minute = second % 60;
    second /= 60;
    realTime->hour = second % 24;
    second /= 24;

    Int64 timestampDay = second + __TIME_DAYS_BEFORE_1970;
    if (timestampDay < 0) {
        timestampDay -= (__TIME_DAYS_IN_ERA - 1);
    }

    int era = timestampDay / __TIME_DAYS_IN_ERA;
    int dayOfEra = timestampDay - era * __TIME_DAYS_IN_ERA;
    int yearOfEra = (dayOfEra - dayOfEra / (__TIME_DAYS_IN_4_YEARS - 1) + dayOfEra / __TIME_DAYS_IN_100_YEARS - dayOfEra / (__TIME_DAYS_IN_ERA - 1)) / 365;
    realTime->year = yearOfEra + era * 400;
    int dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    int month = (5 * dayOfYear + 2) / 153;
    realTime->day = dayOfYear - (153 * month + 2) / 5 + 1;

    if (month < 10) {
        month += 3;
    } else {
        month -= 9;
        ++realTime->year;
    }

    realTime->month = month;
}

void timestamp_step(Timestamp* timestamp, Uint64 time, TimeUnit unit) {
    timestamp->nanosecond += time * unit;
    while (timestamp->nanosecond >= TIME_UNIT_SECOND) {
        timestamp->nanosecond -= TIME_UNIT_SECOND;
        ++timestamp->second;
    }
}

Int64 timestamp_compare(Timestamp* ts1, Timestamp* ts2) {
    return TIME_UNIT_SECOND * (ts1->second - ts2->second) + ((Int64)ts1->nanosecond - (Int64)ts2->nanosecond);
}

typedef struct {
    Timestamp       time;
    Timestamp       expectedTime;
    Spinlock        timeLock;   //TODO: Sequence lock?
    Uint64          lastMainTick;
    Uint64          beatTickTime;
    Int64           timeAdjust;
    ClockSourceType mainClockSource;
    ClockSourceType beatClockSource;
} __Clock;

static __Clock _clock;

ISR_FUNC_HEADER(__time_timerHandler) {  //TODO: This timer is a little slower than expection
    ClockSource* mainClockSource = clockSource_getSource(_clock.mainClockSource), * beatClockSource = clockSource_getSource(_clock.beatClockSource);
    rawClockSourceUpdateTick(beatClockSource);

    spinlock_lock(&_clock.timeLock);
    Timestamp* time = &_clock.time, * expectedTime = &_clock.expectedTime;

    Uint64 currentMainTick = rawClockSourceReadTick(mainClockSource);
    Uint64 dNanosecond = CLOCK_SOURCE_CONVERT_TICK_TO_TIME(mainClockSource, currentMainTick - _clock.lastMainTick, TIME_UNIT_NANOSECOND) + _clock.timeAdjust;
    timestamp_step(time, dNanosecond, TIME_UNIT_NANOSECOND);

    timestamp_step(expectedTime, _clock.beatTickTime, TIME_UNIT_NANOSECOND);
    _clock.timeAdjust = (_clock.timeAdjust + timestamp_compare(expectedTime, time)) >> 1;
    _clock.lastMainTick = currentMainTick;

    spinlock_unlock(&_clock.timeLock);

    if (schedule_getCurrentScheduler()->started) {  //TODO: Ugly code, This code is not independent from scheduler
        schedulerTick();
    }

    timer_updateTimers();
}

Result time_init() {
    clockSources_init();

    ClockSource* CMOSclockSource = clockSource_getSource(CLOCK_SOURCE_TYPE_CMOS), * mainClockSource, * beatClockSource;
    if (TEST_FLAGS_FAIL(CMOSclockSource->flags, CLOCK_SOURCE_FLAGS_PRESENT)) {
        return RESULT_FAIL;
    }

    _clock.time                 = (Timestamp) {
        .second     = CLOCK_SOURCE_CONVERT_TICK_TO_TIME(CMOSclockSource, rawClockSourceReadTick(CMOSclockSource), TIME_UNIT_SECOND),
        .nanosecond = 0
    };
    _clock.expectedTime         = _clock.time;

    _clock.timeLock             = SPINLOCK_UNLOCKED;
    _clock.beatClockSource      = CLOCK_SOURCE_TYPE_I8254;
    _clock.mainClockSource      = CLOCK_SOURCE_TYPE_CPU;

    mainClockSource = clockSource_getSource(_clock.mainClockSource);
    if (TEST_FLAGS_FAIL(mainClockSource->flags, CLOCK_SOURCE_FLAGS_PRESENT | CLOCK_SOURCE_FLAGS_AUTO_UPDATE) || mainClockSource->readTick == NULL) {
        return RESULT_FAIL;
    }

    beatClockSource = clockSource_getSource(_clock.beatClockSource);
    if (TEST_FLAGS_FAIL(beatClockSource->flags, CLOCK_SOURCE_FLAGS_PRESENT) || mainClockSource->readTick == NULL || beatClockSource->updateTick == NULL || beatClockSource->start == NULL) {
        return RESULT_FAIL;
    }

    bool interruptEnabled = idt_disableInterrupt();
    idt_registerISR(0x20, __time_timerHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
    idt_setInterrupt(interruptEnabled);

    _clock.lastMainTick         = rawClockSourceReadTick(mainClockSource);
    _clock.beatTickTime         = TIME_UNIT_SECOND / beatClockSource->hz;
    _clock.timeAdjust           = 0;

    timer_init(beatClockSource);

    if (rawClockSourceStart(beatClockSource) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

void time_getTimestamp(Timestamp* timestamp) {
    spinlock_lock(&_clock.timeLock);
    *timestamp = _clock.time;
    ClockSource* mainClockSource = clockSource_getSource(_clock.mainClockSource);
    Uint64 step = CLOCK_SOURCE_CONVERT_TICK_TO_TIME(mainClockSource, rawClockSourceReadTick(mainClockSource) - _clock.lastMainTick, TIME_UNIT_NANOSECOND) + _clock.timeAdjust;
    spinlock_unlock(&_clock.timeLock);
    timestamp_step(timestamp, step, TIME_UNIT_NANOSECOND);
}
