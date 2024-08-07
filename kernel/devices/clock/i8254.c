#include<devices/clock/i8254.h>

#include<devices/clock/clockSource.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/types.h>
#include<real/ports/PIT.h>
#include<real/simpleAsmLines.h>

#define __I8254_LATCH(__HZ) ((PIT_MAX_FREQUENCY + (__HZ / 2)) / __HZ)

static Uint64 __i8254_readTick(ClockSource* this);

static Result __i8254_updateTick(ClockSource* this);

static Result __i8254_start(ClockSource* this);

static Result __i8254_stop(ClockSource* this);

Result i8254_initClockSource(ClockSource* clockSource) {
    *clockSource = (ClockSource) {
        .type                   = CLOCK_SOURCE_TYPE_I8254,
        .tick                   = 0,
        .hz                     = CLOCK_SOURCE_I8254_DEFAULT_FREQUENCY,
        .tickConvertMultiplier  = CLOCK_SOURCE_HZ_TO_TICK_CONVERT_MULTIPLER(CLOCK_SOURCE_I8254_DEFAULT_FREQUENCY),
        .lock                   = SPINLOCK_UNLOCKED,
        .flags                  = CLOCK_SOURCE_FLAGS_PRESENT,
        .readTick               = __i8254_readTick,
        .updateTick             = __i8254_updateTick,
        .start                  = __i8254_start,
        .stop                   = __i8254_stop
    };

    return RESULT_SUCCESS;
}

static Uint64 __i8254_readTick(ClockSource* this) {
    spinlock_lock(&this->lock);
    Uint64 ret = this->tick;
    spinlock_unlock(&this->lock);
    return ret;
}

static Result __i8254_updateTick(ClockSource* this) {
    spinlock_lock(&this->lock);
    ++this->tick;
    spinlock_unlock(&this->lock);
    return RESULT_SUCCESS;
}

static Result __i8254_start(ClockSource* this) {
    spinlock_lock(&this->lock);
    outb(PIT_CONTROL, PIT_CONTROL_CHANNEL_SELECT(0) | PIT_CONTROL_BITS_MASK_LOW_HIGH_SEP | PIT_CONTROL_MODE_RATE_GENERATOR);
    Uint16 latch = __I8254_LATCH(this->hz);
    outb(PIT_CHANNEL_SELECT(0), EXTRACT_VAL(latch, 16, 0, 8));
    outb(PIT_CHANNEL_SELECT(0), EXTRACT_VAL(latch, 16, 8, 16));
    spinlock_unlock(&this->lock);
    return RESULT_SUCCESS;
}

static Result __i8254_stop(ClockSource* this) {
    spinlock_lock(&this->lock);
    outb(PIT_CONTROL, PIT_CONTROL_CHANNEL_SELECT(0) | PIT_CONTROL_BITS_MASK_LOW_HIGH_SEP | PIT_CONTROL_MODE_INTERRUPT_ON_TERMINAL_COUNT);
    spinlock_unlock(&this->lock);
    return RESULT_SUCCESS;
}