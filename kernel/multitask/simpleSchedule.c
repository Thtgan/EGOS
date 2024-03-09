#include<multitask/simpleSchedule.h>

#include<debug.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/physicalPages.h>
#include<multitask/process.h>
#include<multitask/spinlock.h>
#include<structs/queue.h>
#include<structs/singlyLinkedList.h>

typedef struct {
    Queue statusQueues[PROCESS_STATUS_NUM];
    Spinlock queueLock;

    Scheduler scheduler;
} __SimpleScheduler;

Result __start(Scheduler* this, Process* initProcess);

void __tick(Scheduler* this);

void __yield(Scheduler* this);

Result __addProcess(Scheduler* this, Process* process);

Result __terminateProcess(Scheduler* this, Process* process);

Result __blockProcess(Scheduler* this, Process* process);

Result __wakeProcess(Scheduler* this, Process* process);

/**
 * @brief Schedule the process
 * 
 * @param scheduler Simple scheduler in use
 * @param newStatus New status current process will be switched to
 */
static void __schedule(__SimpleScheduler* scheduler, ProcessStatus newStatus);

/**
 * @brief Change the status of process, and add it to the end of corresponding queue
 * 
 * @param scheduler Simple scheduler in use
 * @param process Process
 * @param status Process status
 */
static void __setProcessStatus(__SimpleScheduler* scheduler, Process* process, ProcessStatus status);

/**
 * @brief Get head process of status queue
 * 
 * @param scheduler Simple scheduler in use
 * @param status Process status
 * @return Process* Process at the head of status queue, NULL if head is not exist
 */
static Process* __getStatusQueueHead(__SimpleScheduler* scheduler, ProcessStatus status);

/**
 * @brief Remove process from the queue holds the process
 * 
 * @param scheduler Simple scheduler in use
 * @param process Process to remove
 * @return Result Result of the operation
 */
static Result __removeProcessFromQueue(__SimpleScheduler* scheduler, Process* process);

Scheduler* createSimpleScheduler() {    //TODO: Scheduler found may stuck
    __SimpleScheduler* ret = kMallocSpecific(sizeof(__SimpleScheduler), PHYSICAL_PAGE_ATTRIBUTE_PUBLIC, 16);    //Share between processes
    if (ret == NULL) {
        return NULL;
    }

    for (int i = 0; i < PROCESS_STATUS_NUM; ++i) {
        queue_initStruct(&ret->statusQueues[i]);
    }

    ret->queueLock = SPINLOCK_UNLOCKED;

    ret->scheduler = (Scheduler) {
        .start = __start,
        .started = false,
        .tick = __tick,
        .tickCnt = 0,
        .yield = __yield,
        .addProcess = __addProcess,
        .terminateProcess = __terminateProcess,
        .blockProcess = __blockProcess,
        .wakeProcess = __wakeProcess,
        .currentProcess = NULL
    };

    return &ret->scheduler;
}

Result __start(Scheduler* this, Process* initProcess) {
    this->started = true;
    __setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), initProcess, PROCESS_STATUS_RUNNING);
}

void __tick(Scheduler* this) {
    if (!this->started) {
        return;
    }

    ++this->tickCnt;
    Process* p = this->currentProcess;
    if (--p->remainTick == 0) {
        p->remainTick = PROCESS_TICK;
        schedulerYield();
    }
}

void __yield(Scheduler* this) {
    if (!this->started) {
        return;
    }

    __schedule(HOST_POINTER(this, __SimpleScheduler, scheduler), PROCESS_STATUS_READY);
}

Result __addProcess(Scheduler* this, Process* process) {
    if (!this->started) {
        return RESULT_FAIL;
    }
    __setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), process, PROCESS_STATUS_READY);
    return RESULT_SUCCESS;
}

Result __terminateProcess(Scheduler* this, Process* process) {
    if (!this->started) {
        return RESULT_FAIL;
    }

    if (process == schedulerGetCurrentProcess(this)) {
        __schedule(HOST_POINTER(this, __SimpleScheduler, scheduler), PROCESS_STATUS_DYING);
        debug_blowup("Terminated process still alive\n");
    }

    __setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), process, PROCESS_STATUS_DYING);
    return RESULT_SUCCESS;
}

Result __blockProcess(Scheduler* this, Process* process) {
    if (!this->started) {
        return RESULT_FAIL;
    }

    if (process == schedulerGetCurrentProcess(this)) {
        __schedule(HOST_POINTER(this, __SimpleScheduler, scheduler), PROCESS_STATUS_WAITING);

        return RESULT_SUCCESS;
    }

    __setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), process, PROCESS_STATUS_WAITING);
    return RESULT_SUCCESS;
}

Result __wakeProcess(Scheduler* this, Process* process) {
    if (!this->started) {
        return RESULT_FAIL;
    }

    __setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), process, PROCESS_STATUS_READY);
    return RESULT_SUCCESS;
}

static void __schedule(__SimpleScheduler* scheduler, ProcessStatus newStatus) {
    Process* current = scheduler->scheduler.currentProcess, * next = __getStatusQueueHead(scheduler, PROCESS_STATUS_READY);

    if (next != NULL) {
        __setProcessStatus(scheduler, current, newStatus);
        __setProcessStatus(scheduler, next, PROCESS_STATUS_RUNNING);

        if (current != next) {
            switchProcess(current, next);
        }
    }

    while (!queue_isEmpty(&scheduler->statusQueues[PROCESS_STATUS_DYING])) {
        Process* dyingProcess = __getStatusQueueHead(scheduler, PROCESS_STATUS_DYING);
        __removeProcessFromQueue(scheduler, dyingProcess);
        releaseProcess(dyingProcess);
    }
}

static void __setProcessStatus(__SimpleScheduler* scheduler, Process* process, ProcessStatus status) {
    if (process->status == status) {
        return;
    }

    if (process->status != PROCESS_STATUS_UNKNOWN && __removeProcessFromQueue(scheduler, process) == RESULT_FAIL) {
        debug_blowup("Remove process from queue failed\n");
    }

    process->status = status;

    spinlockLock(&scheduler->queueLock);

    queueNode_initStruct(&process->statusQueueNode);
    queue_push(&scheduler->statusQueues[status], &process->statusQueueNode);

    spinlockUnlock(&scheduler->queueLock);

    if (status == PROCESS_STATUS_RUNNING) {
        scheduler->scheduler.currentProcess = process;
    }
}

static Process* __getStatusQueueHead(__SimpleScheduler* scheduler, ProcessStatus status) {
    spinlockLock(&scheduler->queueLock);
    Process* ret = queue_isEmpty(&scheduler->statusQueues[status]) ? NULL : HOST_POINTER(queue_front(&scheduler->statusQueues[status]), Process, statusQueueNode);
    spinlockUnlock(&scheduler->queueLock);

    return ret;
}

static Result __removeProcessFromQueue(__SimpleScheduler* scheduler, Process* process) {
    ProcessStatus status = process->status;

    spinlockLock(&scheduler->queueLock);
    QueueNode* nodeAddr = &process->statusQueueNode;
    Queue* queue = &scheduler->statusQueues[status];
    for (QueueNode* i = &queue->q; i->next != &queue->q; i = i->next) {
        void* next = i->next;
        if (next == nodeAddr) {
            singlyLinkedList_deleteNext(i);

            if (next == queue->qTail) {
                queue->qTail = i;
            }

            queueNode_initStruct(&process->statusQueueNode);

            spinlockUnlock(&scheduler->queueLock);

            return RESULT_SUCCESS;
        }
    }

    spinlockUnlock(&scheduler->queueLock);

    return RESULT_FAIL;
}