#if !defined(__SCHEDULE_H)
#define __SCHEDULE_H

#include<kit/types.h>
#include<kit/oop.h>
#include<multitask/process.h>

STRUCT_PRE_DEFINE(Scheduler);

STRUCT_PRIVATE_DEFINE(Scheduler) {
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
};

/**
 * @brief Initialize the schedule
 * 
 * @return Result Result of the operation
 */
Result initSchedule();

/**
 * @brief Get current scheduler
 * 
 * @return Current scheduler in use
 */
Scheduler* getScheduler();

/**
 * @brief Wrapper function of scheduler start
 * 
 * @param initProcess Initial process
 * @return Result Result of the operation
 */
static inline Result schedulerStart(Process* initProcess) {
    Scheduler* scheduler = getScheduler();
    return scheduler->start(scheduler, initProcess);
}

/**
 * @brief Wrapper function of scheduler tick
 */
static inline void schedulerTick() {
    Scheduler* scheduler = getScheduler();
    scheduler->tick(scheduler);
}

/**
 * @brief Wrapper function of scheduler yield
 */
static inline void schedulerYield() {
    Scheduler* scheduler = getScheduler();
    scheduler->yield(scheduler);
}

/**
 * @brief Wrapper function of scheduler add process
 * 
 * @param process Process
 * @return Result Result of the operation
 */
static inline Result schedulerAddProcess(Process* process) {
    Scheduler* scheduler = getScheduler();
    return scheduler->addProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler terminate process
 * 
 * @param process Process
 * @return Result Result of the operation
 */
static inline Result schedulerTerminateProcess(Process* process) {
    Scheduler* scheduler = getScheduler();
    return scheduler->terminateProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler block process
 * 
 * @param process Process
 * @return Result Result of the operation
 */
static inline Result schedulerBlockProcess(Process* process) {
    Scheduler* scheduler = getScheduler();
    return scheduler->blockProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler wake process
 * 
 * @param process Process
 * @return Result Result of the operation
 */
static inline Result schedulerWakeProcess(Process* process) {
    Scheduler* scheduler = getScheduler();
    return scheduler->wakeProcess(scheduler, process);
}

/**
 * @brief Wrapper function of scheduler get current process
 * 
 * @return Process* current process running
 */
static inline Process* schedulerGetCurrentProcess() {
    Scheduler* scheduler = getScheduler();
    return scheduler->currentProcess;
}

#endif // __SCHEDULE_H
