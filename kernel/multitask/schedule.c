#include<multitask/schedule.h>

#include<debug.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<multitask/process.h>
#include<multitask/spinlock.h>
#include<structs/queue.h>
#include<structs/singlyLinkedList.h>

static Queue statusQueues[PROCESS_STATUS_NUM];

static Spinlock _queueLock = SPINLOCK_UNLOCKED;

/**
 * @brief Remove process from the queue holds the process
 * 
 * @param process Process to remove
 * @return Result Result of the operation
 */
static Result __removeProcessFromQueue(Process* process);

void schedule(ProcessStatus newStatus) {
    Process* current = getCurrentProcess(), * next = getStatusQueueHead(PROCESS_STATUS_READY);

    if (next != NULL) {
        setProcessStatus(current, newStatus);
        setProcessStatus(next, PROCESS_STATUS_RUNNING);

        if (current != next) {
            switchProcess(current, next);
        }
    }

    while (!isQueueEmpty(&statusQueues[PROCESS_STATUS_DYING])) {
        Process* dyingProcess = getStatusQueueHead(PROCESS_STATUS_DYING);
        __removeProcessFromQueue(dyingProcess);
        releaseProcess(dyingProcess);
    }
}

Result initSchedule() {
    for (int i = 0; i < PROCESS_STATUS_NUM; ++i) {
        initQueue(&statusQueues[i]);
    }

    initTSS();
    initProcess();
    if (forkFromCurrentProcess("Idle") == NULL) {
        idle();
    }

    return RESULT_SUCCESS;
}

void setProcessStatus(Process* process, ProcessStatus status) {
    if (process->status == status) {
        return;
    }

    spinlockLock(&_queueLock);

    if (process->status != PROCESS_STATUS_UNKNOWN && !__removeProcessFromQueue(process)) {
        blowup("Remove process from queue failed\n");
    }

    process->status = status;

    initQueueNode(&process->statusQueueNode);
    queuePush(&statusQueues[status], &process->statusQueueNode);

    spinlockUnlock(&_queueLock);
}

Process* getStatusQueueHead(ProcessStatus status) {
    spinlockLock(&_queueLock);
    Process* ret = isQueueEmpty(&statusQueues[status]) ? NULL : HOST_POINTER(queueFront(&statusQueues[status]), Process, statusQueueNode);
    spinlockUnlock(&_queueLock);

    return ret;
}

static Result __removeProcessFromQueue(Process* process) {
    ProcessStatus status = process->status;

    QueueNode* nodeAddr = &process->statusQueueNode;
    for (QueueNode* i = &statusQueues[status].q; i->next != &statusQueues[status].q; i = i->next) {
        void* next = i->next;
        if (next == nodeAddr) {
            singlyLinkedListDeleteNext(i);

            if (next == statusQueues[status].qTail) {
                statusQueues[status].qTail = i;
            }

            initQueueNode(&process->statusQueueNode);

            return RESULT_SUCCESS;
        }
    }
    return RESULT_FAIL;
}

void idle() {
    while (true) {
        sti();
        hlt();
    }

    blowup("Idle is trying to return\n");
}