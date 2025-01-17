#include<devices/clock/CMOS.h>

#include<devices/clock/clockSource.h>
#include<kit/types.h>
#include<multitask/locks/spinlock.h>
#include<real/ports/CMOS.h>
#include<real/simpleAsmLines.h>
#include<time/time.h>

static Uint64 __CMOS_readTick(ClockSource* this);

OldResult CMOS_initClockSource(ClockSource* clockSource) {
    *clockSource = (ClockSource) {
        .type                   = CLOCK_SOURCE_TYPE_CMOS,
        .tick                   = 0,
        .hz                     = 1,
        .tickConvertMultiplier  = CLOCK_SOURCE_HZ_TO_TICK_CONVERT_MULTIPLER(1),
        .lock                   = SPINLOCK_UNLOCKED,
        .flags                  = CLOCK_SOURCE_FLAGS_PRESENT | CLOCK_SOURCE_FLAGS_AUTO_UPDATE,
        .readTick               = __CMOS_readTick,
        .updateTick             = NULL,
        .start                  = NULL,
        .stop                   = NULL
    };

    return RESULT_SUCCESS;
}

static inline Uint8 __CMOS_readVal(Uint8 addr) {
    outb(CMOS_INDEX, addr);
    return inb(CMOS_DATA);
}

#define __BCD_TO_BIN(__BCD) ((__BCD >> 4) * 10 + (__BCD & 0xF))

static Uint64 __CMOS_readTick(ClockSource* this) {
    spinlock_lock(&this->lock);

    Uint32 retry = 1000000;
    while (retry-- > 0 && TEST_FLAGS(__CMOS_readVal(CMOS_STATUS_A), CMOS_STATUS_A_RTC_UPGRADING));

    if (retry == 0) {
        return CLOCK_SOURCE_INVALID_TICK;
    }

    RealTime realTime;
    realTime.year   = __CMOS_readVal(CMOS_YEAR);
    realTime.month  = __CMOS_readVal(CMOS_MONTH);
    realTime.day    = __CMOS_readVal(CMOS_DAT_OF_MONTH);
    realTime.hour   = __CMOS_readVal(CMOS_RTC_CURRENT_HOUR);
    realTime.minute = __CMOS_readVal(CMOS_RTC_CURRENT_MINUTE);
    realTime.second = __CMOS_readVal(CMOS_RTC_CURRENT_SECOND);

    spinlock_unlock(&this->lock);

    realTime.year   = __BCD_TO_BIN(realTime.year);
    if (realTime.year < 70) {
        realTime.year += 100;
    }
    realTime.year   += 1900;
    realTime.month  = __BCD_TO_BIN(realTime.month);
    realTime.day    = __BCD_TO_BIN(realTime.day);
    realTime.hour   = __BCD_TO_BIN(realTime.hour);
    realTime.minute = __BCD_TO_BIN(realTime.minute);
    realTime.second = __BCD_TO_BIN(realTime.second);

    realTime.millisecond = realTime.microsecond = realTime.nanosecond = 0;

    Timestamp timestamp;
    time_convertRealTimeToTimestamp(&realTime, &timestamp);

    return timestamp.second;
}
