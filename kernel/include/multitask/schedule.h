#if !defined(__MULTITASK_SCHEDULE_H)
#define __MULTITASK_SCHEDULE_H

typedef struct Scheduler Scheduler;

#include<kit/types.h>
#include<kit/oop.h>
#include<multitask/process.h>

typedef struct Scheduler {
    Result (*start)(Scheduler* this, Process* initProcess);
    bool started;

    void (*tick)(Scheduler* this);
    Uint64 tickCnt;

    void (*yield)(Scheduler* this);
    Result (*addProcess)(Scheduler* this, Process* process);
    Result (*terminateProcess)(Scheduler* this, Process* process);
    Result (*blockProcess)(Scheduler* this, Process* process);
    Result (*wakeProcess)(Scheduler* this, Process* process);

    Process* currentProcess;
} Scheduler;

/**
 * @brief Initialize the schedule
 * 
 * @return Result Result of the operation
 */
Result schedule_init();

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
 * @return Result Result of the operation
 */
static inline Result scheduler_start(Process* initProcess) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    return scheduler->start(scheduler, initProcess);
}

/**
 * @brief Wrapper function of scheduler tick
 */
static inline void scheduler_tick() {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    scheduler->tick(scheduler);
}

/**
 * @brief Wrapper function of scheduler yield
 */
static inline void scheduler_yield() {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    scheduler->yield(scheduler);
}

/**
 * @brief Wrapper function of scheduler add process
 * 
 * @param process Process
 * @return Result Result of the operation
 */
static inline Result scheduler_addProcess(Process* process) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    return scheduler->addProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler terminate process
 * 
 * @param process Process
 * @return Result Result of the operation
 */
static inline Result scheduler_terminateProcess(Process* process) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    return scheduler->terminateProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler block process
 * 
 * @param process Process
 * @return Result Result of the operation
 */
static inline Result scheduler_blockProcess(Process* process) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    return scheduler->blockProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler wake process
 * 
 * @param process Process
 * @return Result Result of the operation
 */
static inline Result scheduler_wakeProcess(Process* process) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    return scheduler->wakeProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler get current process
 * 
 * @return Process* current process running
 */
static inline Process* scheduler_getCurrentProcess() {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    return scheduler->currentProcess;
}

#endif // __MULTITASK_SCHEDULE_H
