#include<multitask/simpleSchedule.h>

#include<debug.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<multitask/criticalToken.h>
#include<multitask/mutex.h>
#include<multitask/process.h>
#include<structs/queue.h>
#include<structs/singlyLinkedList.h>

typedef struct {
    Queue statusQueues[PROCESS_STATUS_NUM];

    Scheduler scheduler;
    // CriticalToken criticalToken;
    Mutex mutex;
} __SimpleScheduler;

Result __simpleScheduler_start(Scheduler* this, Process* initProcess);

void __simpleScheduler_tick(Scheduler* this);

void __simpleScheduler_yield(Scheduler* this);

Result __simpleScheduler_addProcess(Scheduler* this, Process* process);

Result __simpleScheduler_terminateProcess(Scheduler* this, Process* process);

Result __simpleScheduler_blockProcess(Scheduler* this, Process* process);

Result __simpleScheduler_wakeProcess(Scheduler* this, Process* process);

/**
 * @brief Schedule the process
 * 
 * @param scheduler Simple scheduler in use
 * @param newStatus New status current process will be switched to
 */
static void __simpleScheduler_schedule(__SimpleScheduler* scheduler, ProcessStatus newStatus);

/**
 * @brief Change the status of process, and add it to the end of corresponding queue
 * 
 * @param scheduler Simple scheduler in use
 * @param process Process
 * @param status Process status
 */
static void __simpleScheduler_setProcessStatus(__SimpleScheduler* scheduler, Process* process, ProcessStatus status);

/**
 * @brief Get head process of status queue
 * 
 * @param scheduler Simple scheduler in use
 * @param status Process status
 * @return Process* Process at the head of status queue, NULL if head is not exist
 */
static Process* __simpleScheduler_getStatusQueueHead(__SimpleScheduler* scheduler, ProcessStatus status);

/**
 * @brief Remove process from the queue holds the process
 * 
 * @param scheduler Simple scheduler in use
 * @param process Process to remove
 * @return Result Result of the operation
 */
static Result __simpleScheduler_removeProcessFromQueue(__SimpleScheduler* scheduler, Process* process);

Scheduler* simpleScheduler_create() {    //TODO: Scheduler found may stuck
    __SimpleScheduler* ret = memory_allocate(sizeof(__SimpleScheduler));
    if (ret == NULL) {
        return NULL;
    }

    for (int i = 0; i < PROCESS_STATUS_NUM; ++i) {
        queue_initStruct(&ret->statusQueues[i]);
    }

    ret->scheduler = (Scheduler) {
        .start = __simpleScheduler_start,
        .started = false,
        .tick = __simpleScheduler_tick,
        .tickCnt = 0,
        .yield = __simpleScheduler_yield,
        .addProcess = __simpleScheduler_addProcess,
        .terminateProcess = __simpleScheduler_terminateProcess,
        .blockProcess = __simpleScheduler_blockProcess,
        .wakeProcess = __simpleScheduler_wakeProcess,
        .currentProcess = NULL,
        .isrDelayYield = false
    };

    mutex_initStruct(&ret->mutex);

    return &ret->scheduler;
}

Result __simpleScheduler_start(Scheduler* this, Process* initProcess) {
    __simpleScheduler_setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), initProcess, PROCESS_STATUS_RUNNING);
    this->started = true;
}

void __simpleScheduler_tick(Scheduler* this) {
    if (!this->started) {
        return;
    }

    ++this->tickCnt;
    Process* p = this->currentProcess;
    if (--p->remainTick == 0) {
        p->remainTick = PROCESS_TICK;
        scheduler_tryYield(this);
    }
}

void __simpleScheduler_yield(Scheduler* this) {
    if (!this->started) {
        return;
    }

    DEBUG_ASSERT_SILENT(!idt_isInISR(true));
    __simpleScheduler_schedule(HOST_POINTER(this, __SimpleScheduler, scheduler), PROCESS_STATUS_READY);
}

Result __simpleScheduler_addProcess(Scheduler* this, Process* process) {
    if (!this->started) {
        return RESULT_ERROR;
    }
    __simpleScheduler_setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), process, PROCESS_STATUS_READY);
    return RESULT_SUCCESS;
}

Result __simpleScheduler_terminateProcess(Scheduler* this, Process* process) {
    if (!this->started) {
        return RESULT_ERROR;
    }

    __SimpleScheduler* simpleScheduler = HOST_POINTER(this, __SimpleScheduler, scheduler);

    if (process == scheduler_getCurrentProcess(this)) {
        __simpleScheduler_schedule(HOST_POINTER(this, __SimpleScheduler, scheduler), PROCESS_STATUS_DYING);
        debug_blowup("Terminated process still alive\n");
    }

    __simpleScheduler_setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), process, PROCESS_STATUS_DYING);
    return RESULT_SUCCESS;
}

Result __simpleScheduler_blockProcess(Scheduler* this, Process* process) {
    if (!this->started) {
        return RESULT_ERROR;
    }

    if (process == scheduler_getCurrentProcess(this)) {
        __simpleScheduler_schedule(HOST_POINTER(this, __SimpleScheduler, scheduler), PROCESS_STATUS_WAITING);

        return RESULT_SUCCESS;
    }

    __simpleScheduler_setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), process, PROCESS_STATUS_WAITING);
    return RESULT_SUCCESS;
}

Result __simpleScheduler_wakeProcess(Scheduler* this, Process* process) {
    if (!this->started) {
        return RESULT_ERROR;
    }

    __simpleScheduler_setProcessStatus(HOST_POINTER(this, __SimpleScheduler, scheduler), process, PROCESS_STATUS_READY);
    return RESULT_SUCCESS;
}

// static int _test_cnt = 0;

static void __simpleScheduler_schedule(__SimpleScheduler* scheduler, ProcessStatus newStatus) {
    mutex_acquire(&scheduler->mutex, (Object)scheduler, MUTEX_ACQUIRE_FLAG_CRITICAL);

    Process* current = scheduler->scheduler.currentProcess, * next = __simpleScheduler_getStatusQueueHead(scheduler, PROCESS_STATUS_READY);

    if (next != NULL) {
        __simpleScheduler_setProcessStatus(scheduler, current, newStatus);
        __simpleScheduler_setProcessStatus(scheduler, next, PROCESS_STATUS_RUNNING);

        if (current != next) {
            DEBUG_ASSERT_SILENT(!idt_isInISR());
            mutex_release(&scheduler->mutex, (Object)scheduler);
            // if (++_test_cnt == 100) {
            //     print_printf(TERMINAL_LEVEL_OUTPUT, "TEST\n");
            // }
            process_switch(current, next);
            mutex_acquire(&scheduler->mutex, (Object)scheduler, MUTEX_ACQUIRE_FLAG_CRITICAL);
        }
    }

    while (!queue_isEmpty(&scheduler->statusQueues[PROCESS_STATUS_DYING])) {
        Process* dyingProcess = __simpleScheduler_getStatusQueueHead(scheduler, PROCESS_STATUS_DYING);
        __simpleScheduler_removeProcessFromQueue(scheduler, dyingProcess);
        process_release(dyingProcess);
    }
    mutex_release(&scheduler->mutex, (Object)scheduler);
}

static void __simpleScheduler_setProcessStatus(__SimpleScheduler* scheduler, Process* process, ProcessStatus status) {
    if (process->status == status) {
        return;
    }

    mutex_acquire(&scheduler->mutex, (Object)scheduler, MUTEX_ACQUIRE_FLAG_CRITICAL);

    if (process->status != PROCESS_STATUS_UNKNOWN && __simpleScheduler_removeProcessFromQueue(scheduler, process) != RESULT_SUCCESS) {
        debug_blowup("Remove process from queue failed\n");
    }

    process->status = status;

    queueNode_initStruct(&process->statusQueueNode);
    queue_push(&scheduler->statusQueues[status], &process->statusQueueNode);

    if (status == PROCESS_STATUS_RUNNING) {
        scheduler->scheduler.currentProcess = process;
    }

    mutex_release(&scheduler->mutex, (Object)scheduler);
}

static Process* __simpleScheduler_getStatusQueueHead(__SimpleScheduler* scheduler, ProcessStatus status) {
    mutex_acquire(&scheduler->mutex, (Object)scheduler, MUTEX_ACQUIRE_FLAG_CRITICAL);
    Process* ret = queue_isEmpty(&scheduler->statusQueues[status]) ? NULL : HOST_POINTER(queue_front(&scheduler->statusQueues[status]), Process, statusQueueNode);
    mutex_release(&scheduler->mutex, (Object)scheduler);

    return ret;
}

static Result __simpleScheduler_removeProcessFromQueue(__SimpleScheduler* scheduler, Process* process) {
    mutex_acquire(&scheduler->mutex, (Object)scheduler, MUTEX_ACQUIRE_FLAG_CRITICAL);

    ProcessStatus status = process->status;

    QueueNode* nodeAddr = &process->statusQueueNode;
    Queue* queue = &scheduler->statusQueues[status];
    bool ret = RESULT_ERROR;
    for (QueueNode* i = &queue->q; i->next != &queue->q; i = i->next) {
        void* next = i->next;
        if (next == nodeAddr) {
            singlyLinkedList_deleteNext(i);

            if (next == queue->qTail) {
                queue->qTail = i;
            }

            queueNode_initStruct(&process->statusQueueNode);
            
            ret = RESULT_SUCCESS;
            break;
        }
    }

    mutex_release(&scheduler->mutex, (Object)scheduler);

    return ret;
}