#include<devices/timer/PIT.h>

#include<devices/terminal/terminalSwitch.h>
#include<devices/timer/timer.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<multitask/schedule.h>
#include<print.h>
#include<real/ports/PIT.h>
#include<real/simpleAsmLines.h>

static Uint64 _tick, _loopPerTick;

ISR_FUNC_HEADER(__timerHandler) {
    ++_tick;
    schedulerTick();
}

/**
 * @brief Use busy loop for the case that sleep for a period shorter than a tick
 * 
 * @param loop loops to wait
 */
static void __loopWait(Uint64 loop);

/**
 * @brief Test out the num of loop per tick could take, no more than a whole tick;
 * 
 * @return Uint64 Num of loop a tick takes
 */
static Uint64 __findTickLoop();

static void __loopWait(Uint64 loop) {
    while (loop--) {
        nop();
    }
}

static bool __testLoop(Uint64 loop) {
    Uint8 cnt = 0;
    for (int i = 0; i < 5; ++i) {   //Test more than one time for better accuracy
        Uint64 t = _tick;
        while (t == _tick) {
            nop();
        } //Wait for beginning of the another tick

        t = _tick;
        __loopWait(loop);
        cnt += (t != _tick);
    }
    
    return cnt > 2;
}

void initPIT() {
    _tick = 0;

    bool interruptEnabled = disableInterrupt();
    registerISR(0x20, __timerHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
    configureChannel(0, 3, TICK_FREQUENCY);
    setInterrupt(interruptEnabled);

    _loopPerTick = 3800000; //TODO: NO, No magic number here
}

void configureChannel(Uint8 channel, Uint8 mode, Uint32 frequency) {
    Uint16 count;

    if (frequency <= 18) {                      //Minimum frequency PIT could reach is 18.2065Hz
        count = 0;                              //Means 65536 to PIT
    } else if (frequency > PIT_MAX_FREQUENCY) {
        count = 2;                              //1 is illegal
    } else {
        count = (PIT_MAX_FREQUENCY + (frequency >> 1)) / frequency;
    }

    outb(PIT_CONTROL, PIT_CONTROL_CHANNEL_SELECT(channel) | PIT_CONTROL_BITS_MASK_LOW_HIGH_SEP | PIT_CONTROL_MODE_SELECT(mode));
    outb(PIT_CHANNEL_SELECT(channel), EXTRACT_VAL(count, 16, 0, 8));
    outb(PIT_CHANNEL_SELECT(channel), EXTRACT_VAL(count, 16, 8, 16));
}

Uint8 readbackConfiguration(Uint8 channel) {
    outb(PIT_CONTROL, PIT_CONTROL_READBACK | PIT_READBACK_NO_LATCH_COUNT | PIT_READBACK_CHANNEL(channel));

    return inb(PIT_CHANNEL_SELECT(channel));
}

#define TICK_NANOSECOND (SECOND / TICK_FREQUENCY)

void PITsleep(TimeUnit unit, Uint64 time) {
    Uint64 tick = time * unit / TICK_NANOSECOND, remain = time * unit % TICK_NANOSECOND;

    Uint64 targetTick = _tick + tick;
    while (_tick < targetTick) {
        nop();
    }

    if (remain > 0) {
        __loopWait(remain * _loopPerTick / TICK_NANOSECOND);
    }
}