#if !defined(__MULTITASK_SCHEDULE_H)
#define __MULTITASK_SCHEDULE_H

typedef struct Scheduler Scheduler;

#include<kit/atomic.h>
#include<kit/types.h>
#include<kit/oop.h>
#include<multitask/process.h>

typedef struct Scheduler {
    OldResult (*start)(Scheduler* this, Process* initProcess);
    bool started;

    void (*tick)(Scheduler* this);
    Uint64 tickCnt;

    void (*yield)(Scheduler* this);
    OldResult (*addProcess)(Scheduler* this, Process* process);
    OldResult (*terminateProcess)(Scheduler* this, Process* process);
    OldResult (*blockProcess)(Scheduler* this, Process* process);
    OldResult (*wakeProcess)(Scheduler* this, Process* process);

    Process* currentProcess;

    bool isrDelayYield;
    Uint32 criticalCount;
} Scheduler;

/**
 * @brief Initialize the schedule
 *
 * @return OldResult OldResult of the operation
 */
OldResult schedule_init();

/**
 * @brief Get current scheduler
 *
 * @return Current scheduler in use
 */
Scheduler* schedule_getCurrentScheduler();

/**
 * @brief Wrapper function of scheduler start
 *
 * @param initProcess Initial process
 * @return OldResult OldResult of the operation
 */
static inline OldResult scheduler_start(Scheduler* scheduler, Process* initProcess) {
    return scheduler->start(scheduler, initProcess);
}

/**
 * @brief Wrapper function of scheduler tick
 */
static inline void scheduler_tick(Scheduler* scheduler) {
    scheduler->tick(scheduler);
}

/**
 * @brief Wrapper function of scheduler yield
 */
static inline void scheduler_yield(Scheduler* scheduler) {
    scheduler->yield(scheduler);
}

/**
 * @brief Wrapper function of scheduler add process
 *
 * @param process Process
 * @return OldResult OldResult of the operation
 */
static inline OldResult scheduler_addProcess(Scheduler* scheduler, Process* process) {
    return scheduler->addProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler terminate process
 *
 * @param process Process
 * @return OldResult OldResult of the operation
 */
static inline OldResult scheduler_terminateProcess(Scheduler* scheduler, Process* process) {
    return scheduler->terminateProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler block process
 *
 * @param process Process
 * @return OldResult OldResult of the operation
 */
static inline OldResult scheduler_blockProcess(Scheduler* scheduler, Process* process) {
    return scheduler->blockProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler wake process
 *
 * @param process Process
 * @return OldResult OldResult of the operation
 */
static inline OldResult scheduler_wakeProcess(Scheduler* scheduler, Process* process) {
    return scheduler->wakeProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler get current process
 *
 * @return Process* current process running
 */
static inline Process* scheduler_getCurrentProcess(Scheduler* scheduler) {
    return scheduler->currentProcess;
}

void scheduler_tryYield(Scheduler* scheduler);

void scheduler_isrDelayYield(Scheduler* scheduler);

static inline void scheduler_enterCritical(Scheduler* scheduler) {
    cli();
    ATOMIC_INC_FETCH(&scheduler->criticalCount);
}

static inline void scheduler_leaveCritical(Scheduler* scheduler) {
    if (ATOMIC_DEC_FETCH(&scheduler->criticalCount) == 0) {
        sti();
    }
}

#endif // __MULTITASK_SCHEDULE_H
