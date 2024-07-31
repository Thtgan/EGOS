#include<time/timer.h>

#include<algorithms.h>
#include<devices/clock/clockSource.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<multitask/schedule.h>
#include<structs/heap.h>
#include<time/time.h>

static ClockSource* _timerClockSource;
static Heap _timerHeap;
#define __TIMER_MAXIMUM_NUM 1023
static Object _heapObjects[1023 + 1];

static Int64 __timers_compare(Object o1, Object o2);

void initTimers(ClockSource* timerClockSource) {
    _timerClockSource = timerClockSource;
    heap_initStruct(&_timerHeap, _heapObjects, __TIMER_MAXIMUM_NUM, __timers_compare);
}

void initTimer(Timer* timer, Int64 time, TimeUnit unit) {
    timer->tick     = umax64(1, CLOCK_SOURCE_CONVERT_TIME_TO_TICK(_timerClockSource, time, unit));  //Wait for 1 tick at least
    timer->until    = -1;
    timer->data     = OBJECT_NULL;
    timer->handler  = NULL;
    timer->flags    = EMPTY_FLAGS;
}

Result timerStart(Timer* timer) {
    if (TEST_FLAGS(timer->flags, TIMER_FLAGS_PRESENT)) {
        return RESULT_FAIL;
    }

    timer->until    = rawClockSourceReadTick(_timerClockSource) + timer->tick;
    heap_push(&_timerHeap, (Object)timer);
    SET_FLAG_BACK(timer->flags, TIMER_FLAGS_PRESENT);
    
    if (TEST_FLAGS(timer->flags, TIMER_FLAGS_SYNCHRONIZE)) {
        while (TEST_FLAGS(timer->flags, TIMER_FLAGS_PRESENT)) { //TODO: Lock?
            schedulerYield();
        }
    }

    return RESULT_SUCCESS;
}
                                                                                                                                                                                                                                                                                                                                                        
Result timerStop(Timer* timer) {
    if (TEST_FLAGS_FAIL(timer->flags, TIMER_FLAGS_PRESENT)) {
        return RESULT_FAIL;
    }
    
    CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_PRESENT);
    return RESULT_SUCCESS;
}

void updateTimers() {
    Timer* topTimer;
    Uint64 currentTick = rawClockSourceReadTick(_timerClockSource);
    while (_timerHeap.size > 0 && heap_top(&_timerHeap, (Object*)&topTimer) == RESULT_SUCCESS && topTimer->until <= currentTick) {
        if (TEST_FLAGS(topTimer->flags, TIMER_FLAGS_PRESENT)) {
            CLEAR_FLAG_BACK(topTimer->flags, TIMER_FLAGS_PRESENT);
            if (topTimer->handler != NULL) {
                topTimer->handler(topTimer);
            }
            
            heap_pop(&_timerHeap);

            if (TEST_FLAGS(topTimer->flags, TIMER_FLAGS_REPEAT)) {
                topTimer->until = currentTick + topTimer->tick;
                SET_FLAG_BACK(topTimer->flags, TIMER_FLAGS_PRESENT);
                heap_push(&_timerHeap, (Object)topTimer);
            }
        } else {
            heap_pop(&_timerHeap);
        }
    }
}

static Int64 __timers_compare(Object o1, Object o2) {
    return ((Timer*)o1)->until - ((Timer*)o2)->until;
}