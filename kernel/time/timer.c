#include<time/timer.h>

#include<algorithms.h>
#include<devices/clock/clockSource.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<multitask/schedule.h>
#include<structs/heap.h>
#include<time/time.h>

static ClockSource* _timer_vlockSource;
static Heap _timer_waitHeap;
#define __TIMER_MAXIMUM_NUM 1023
static Object _timer_heapObjects[1023 + 1];

static Int64 __timer_compare(Object o1, Object o2);

void timer_init(ClockSource* timerClockSource) {
    _timer_vlockSource = timerClockSource;
    heap_initStruct(&_timer_waitHeap, _timer_heapObjects, __TIMER_MAXIMUM_NUM, __timer_compare);
}

void timer_initStruct(Timer* timer, Int64 time, TimeUnit unit) {
    timer->tick     = algorithms_umax64(1, CLOCK_SOURCE_CONVERT_TIME_TO_TICK(_timer_vlockSource, time, unit));  //Wait for 1 tick at least
    timer->until    = -1;
    timer->data     = OBJECT_NULL;
    timer->handler  = NULL;
    timer->flags    = EMPTY_FLAGS;
}

Result timer_start(Timer* timer) {
    if (TEST_FLAGS(timer->flags, TIMER_FLAGS_PRESENT)) {
        return RESULT_FAIL;
    }

    timer->until    = rawClockSourceReadTick(_timer_vlockSource) + timer->tick;
    heap_push(&_timer_waitHeap, (Object)timer);
    SET_FLAG_BACK(timer->flags, TIMER_FLAGS_PRESENT);
    
    if (TEST_FLAGS(timer->flags, TIMER_FLAGS_SYNCHRONIZE)) {
        while (TEST_FLAGS(timer->flags, TIMER_FLAGS_PRESENT)) { //TODO: Lock?
            scheduler_yield();
        }
    }

    return RESULT_SUCCESS;
}
                                                                                                                                                                                                                                                                                                                                                        
Result timer_stop(Timer* timer) {
    if (TEST_FLAGS_FAIL(timer->flags, TIMER_FLAGS_PRESENT)) {
        return RESULT_FAIL;
    }
    
    CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_PRESENT);
    return RESULT_SUCCESS;
}

void timer_updateTimers() {
    Timer* topTimer;
    Uint64 currentTick = rawClockSourceReadTick(_timer_vlockSource);
    while (_timer_waitHeap.size > 0 && heap_top(&_timer_waitHeap, (Object*)&topTimer) == RESULT_SUCCESS && topTimer->until <= currentTick) {
        if (TEST_FLAGS(topTimer->flags, TIMER_FLAGS_PRESENT)) {
            CLEAR_FLAG_BACK(topTimer->flags, TIMER_FLAGS_PRESENT);
            if (topTimer->handler != NULL) {
                topTimer->handler(topTimer);
            }
            
            heap_pop(&_timer_waitHeap);

            if (TEST_FLAGS(topTimer->flags, TIMER_FLAGS_REPEAT)) {
                topTimer->until = currentTick + topTimer->tick;
                SET_FLAG_BACK(topTimer->flags, TIMER_FLAGS_PRESENT);
                heap_push(&_timer_waitHeap, (Object)topTimer);
            }
        } else {
            heap_pop(&_timer_waitHeap);
        }
    }
}

static Int64 __timer_compare(Object o1, Object o2) {
    return ((Timer*)o1)->until - ((Timer*)o2)->until;
}