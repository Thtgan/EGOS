#include<devices/clock/i8254.h>

#include<devices/clock/clockSource.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/types.h>
#include<real/ports/PIT.h>
#include<real/simpleAsmLines.h>

#define __I8254_LATCH(__HZ) ((PIT_MAX_FREQUENCY + (__HZ / 2)) / __HZ)

static Uint64 __i8254ReadTick(ClockSource* this);

static Result __i8254UpdateTick(ClockSource* this);

static Result __i8254Start(ClockSource* this);

static Result __i8254Stop(ClockSource* this);

Result initClockSourceI8254(ClockSource* clockSource) {
    *clockSource = (ClockSource) {
        .type                   = CLOCK_SOURCE_TYPE_I8254,
        .tick                   = 0,
        .hz                     = CLOCK_SOURCE_I8254_DEFAULT_FREQUENCY,
        .tickConvertMultiplier  = CLOCK_SOURCE_HZ_TO_TICK_CONVERT_MULTIPLER(CLOCK_SOURCE_I8254_DEFAULT_FREQUENCY),
        .lock                   = SPINLOCK_UNLOCKED,
        .flags                  = CLOCK_SOURCE_FLAGS_PRESENT,
        .readTick               = __i8254ReadTick,
        .updateTick             = __i8254UpdateTick,
        .start                  = __i8254Start,
        .stop                   = __i8254Stop
    };

    return RESULT_SUCCESS;
}

static Uint64 __i8254ReadTick(ClockSource* this) {
    spinlockLock(&this->lock);
    Uint64 ret = this->tick;
    spinlockUnlock(&this->lock);
    return ret;
}

static Result __i8254UpdateTick(ClockSource* this) {
    spinlockLock(&this->lock);
    ++this->tick;
    spinlockUnlock(&this->lock);
    return RESULT_SUCCESS;
}

static Result __i8254Start(ClockSource* this) {
    spinlockLock(&this->lock);
    outb(PIT_CONTROL, PIT_CONTROL_CHANNEL_SELECT(0) | PIT_CONTROL_BITS_MASK_LOW_HIGH_SEP | PIT_CONTROL_MODE_RATE_GENERATOR);
    Uint16 latch = __I8254_LATCH(this->hz);
    outb(PIT_CHANNEL_SELECT(0), EXTRACT_VAL(latch, 16, 0, 8));
    outb(PIT_CHANNEL_SELECT(0), EXTRACT_VAL(latch, 16, 8, 16));
    spinlockUnlock(&this->lock);
    return RESULT_SUCCESS;
}

static Result __i8254Stop(ClockSource* this) {
    spinlockLock(&this->lock);
    outb(PIT_CONTROL, PIT_CONTROL_CHANNEL_SELECT(0) | PIT_CONTROL_BITS_MASK_LOW_HIGH_SEP | PIT_CONTROL_MODE_INTERRUPT_ON_TERMINAL_COUNT);
    spinlockUnlock(&this->lock);
    return RESULT_SUCCESS;
}