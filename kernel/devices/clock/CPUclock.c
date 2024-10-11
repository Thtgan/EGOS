#include<devices/clock/CPUclock.h>

#include<devices/clock/clockSource.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<real/ports/PIT.h>
#include<real/simpleAsmLines.h>

Uint64 __CPUclock_readTick(ClockSource* this);

#define __CPU_CLOCK_CALIBRATE_I8254_HZ      20  //The largest latch i8254's count register can holds, for more accurate calibrate
#define __CPU_CLOCK_CALIBRATE_TEST_ROUND    5   //The largest latch i8254's count register can holds, for more accurate calibrate

Result CPUclock_initClockSource(ClockSource* clockSource) {
    ClockSourceType calibrateClockSourceType = CLOCK_SOURCE_TYPE_I8254;
    ClockSource* calibrateClockSource = clockSource_getSource(calibrateClockSourceType);
    if (TEST_FLAGS_FAIL(calibrateClockSource->flags, CLOCK_SOURCE_FLAGS_PRESENT)) {
        return RESULT_ERROR;
    }

    //TODO: Add support for HPET
    Uint64 hz = 0;
    Uint16 latch = PIT_MAX_FREQUENCY / __CPU_CLOCK_CALIBRATE_I8254_HZ;
    Uint64 begin, end;
    for (int i = 0; i < __CPU_CLOCK_CALIBRATE_TEST_ROUND; ++i) {
        outb(0x61, SET_FLAG(CLEAR_FLAG(inb(0x61), FLAG8(1)), FLAG8(0)));    //This magic comes from old linux, I don't know does it fit modern system
        outb(PIT_CONTROL, PIT_CONTROL_MODE_INTERRUPT_ON_TERMINAL_COUNT | PIT_CONTROL_BITS_MASK_LOW_HIGH_SEP | PIT_CONTROL_CHANNEL_SELECT(2));

        outb(PIT_CHANNEL_SELECT(2), EXTRACT_VAL(latch, 16, 0, 8));
        outb(PIT_CHANNEL_SELECT(2), EXTRACT_VAL(latch, 16, 8, 16));

        begin = rdtsc();
        while (TEST_FLAGS_FAIL(inb(0x61), FLAG8(5)));
        end = rdtsc();

        hz += (end - begin);
    }

    hz = hz * __CPU_CLOCK_CALIBRATE_I8254_HZ / __CPU_CLOCK_CALIBRATE_TEST_ROUND;    //Take average value

    *clockSource = (ClockSource) {
        .type                   = CLOCK_SOURCE_TYPE_CPU,
        .tick                   = 0,
        .hz                     = hz,
        .tickConvertMultiplier  = CLOCK_SOURCE_HZ_TO_TICK_CONVERT_MULTIPLER(hz),
        .lock                   = SPINLOCK_UNLOCKED,
        .flags                  = CLOCK_SOURCE_FLAGS_PRESENT | CLOCK_SOURCE_FLAGS_AUTO_UPDATE,
        .readTick               = __CPUclock_readTick,
        .updateTick             = NULL,
        .start                  = NULL,
        .stop                   = NULL
    };

    return RESULT_SUCCESS;
}

Uint64 __CPUclock_readTick(ClockSource* this) {
    return rdtsc();
}