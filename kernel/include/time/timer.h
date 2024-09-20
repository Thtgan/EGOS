#if !defined(__TIME_TIMER_H)
#define __TIME_TIMER_H

#include<devices/clock/clockSource.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/heap.h>
#include<time/time.h>

STRUCT_PRE_DEFINE(Timer);

typedef void (*TimerHandler)(Timer* tiemr);

STRUCT_PRIVATE_DEFINE(Timer) {
    Int64           tick;   //TODO: Overflow?
    Int64           until;
    TimerHandler    handler;
    Object          data;
    Flags8          flags;
#define TIMER_FLAGS_PRESENT     FLAG8(0)
#define TIMER_FLAGS_REPEAT      FLAG8(1)
#define TIMER_FLAGS_SYNCHRONIZE FLAG8(2)
};

void timer_init(ClockSource* timerClockSource);

void timer_initStruct(Timer* timer, Int64 time, TimeUnit unit);

Result timer_start(Timer* timer);

Result timer_stop(Timer* timer);

void timer_updateTimers();

#endif // __TIME_TIMER_H
