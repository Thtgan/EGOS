#include<time/timer.h>

#include<algorithms.h>
#include<devices/clock/clockSource.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<multitask/locks/mutex.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<structs/heap.h>
#include<time/time.h>
#include<error.h>

static ClockSource* _timer_vlockSource;
static Heap _timer_waitHeap;
#define __TIMER_MAXIMUM_NUM 1023
static Object _timer_heapObjects[1023 + 1];
static Mutex _timer_heapMutex;

static Int64 __timer_compare(Object o1, Object o2);

void timer_init(ClockSource* timerClockSource) {
    _timer_vlockSource = timerClockSource;
    heap_initStruct(&_timer_waitHeap, _timer_heapObjects, __TIMER_MAXIMUM_NUM, __timer_compare);
    mutex_initStruct(&_timer_heapMutex, MUTEX_FLAG_CRITICAL);
}

void timer_initStruct(Timer* timer, Int64 time, TimeUnit unit) {
    timer->tick     = algorithms_umax64(1, CLOCK_SOURCE_CONVERT_TIME_TO_TICK(_timer_vlockSource, time, unit));  //Wait for 1 tick at least
    timer->until    = -1;
    timer->data     = OBJECT_NULL;
    timer->handler  = NULL;
    timer->flags    = EMPTY_FLAGS;
}

void timer_start(Timer* timer) {
    if (TEST_FLAGS(timer->flags, TIMER_FLAGS_PRESENT)) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    timer->until    = rawClockSourceReadTick(_timer_vlockSource) + timer->tick;
    Scheduler* scheduler = schedule_getCurrentScheduler();
    Process* process = scheduler_getCurrentProcess(scheduler);

    mutex_acquire(&_timer_heapMutex, (Object)process);
    heap_push(&_timer_waitHeap, (Object)timer);
    mutex_release(&_timer_heapMutex, (Object)process);
    ERROR_GOTO_IF_ERROR(0);
    SET_FLAG_BACK(timer->flags, TIMER_FLAGS_PRESENT);
    
    if (TEST_FLAGS(timer->flags, TIMER_FLAGS_SYNCHRONIZE)) {
        while (TEST_FLAGS(timer->flags, TIMER_FLAGS_PRESENT)) { //TODO: Lock?
            scheduler_tryYield(schedule_getCurrentScheduler());
        }
    }

    return;
    ERROR_FINAL_BEGIN(0);
}
                                                                                                                                                                                                                                                                                                                                                        
void timer_stop(Timer* timer) {
    if (TEST_FLAGS_FAIL(timer->flags, TIMER_FLAGS_PRESENT)) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }
    
    CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_PRESENT);
    return;
    ERROR_FINAL_BEGIN(0);
}

void timer_updateTimers() {
    Timer* topTimer;
    Uint64 currentTick = rawClockSourceReadTick(_timer_vlockSource);
    Scheduler* scheduler = schedule_getCurrentScheduler();
    Process* process = scheduler_getCurrentProcess(scheduler);
    while (_timer_waitHeap.size > 0) {
        mutex_acquire(&_timer_heapMutex, (Object)process);
        heap_top(&_timer_waitHeap, (Object*)&topTimer);
        ERROR_GOTO_IF_ERROR(0);
        if (topTimer->until > currentTick) {
            mutex_release(&_timer_heapMutex, (Object)process);
            break;
        }
        
        if (TEST_FLAGS(topTimer->flags, TIMER_FLAGS_PRESENT)) {
            CLEAR_FLAG_BACK(topTimer->flags, TIMER_FLAGS_PRESENT);
            if (topTimer->handler != NULL) {
                mutex_release(&_timer_heapMutex, (Object)process);
                topTimer->handler(topTimer);
                mutex_acquire(&_timer_heapMutex, (Object)process);
            }
            
            heap_pop(&_timer_waitHeap);
            ERROR_GOTO_IF_ERROR(0);

            if (TEST_FLAGS(topTimer->flags, TIMER_FLAGS_REPEAT)) {
                topTimer->until = currentTick + topTimer->tick;
                SET_FLAG_BACK(topTimer->flags, TIMER_FLAGS_PRESENT);
                heap_push(&_timer_waitHeap, (Object)topTimer);
                ERROR_GOTO_IF_ERROR(0);
            }
        } else {
            heap_pop(&_timer_waitHeap);
            ERROR_GOTO_IF_ERROR(0);
        }

        mutex_release(&_timer_heapMutex, (Object)process);
    }

    return;
    ERROR_FINAL_BEGIN(0);
    if (mutex_isLocked(&_timer_heapMutex)) {
        mutex_release(&_timer_heapMutex, (Object)scheduler);
    }
}

static Int64 __timer_compare(Object o1, Object o2) {
    return ((Timer*)o1)->until - ((Timer*)o2)->until;
}