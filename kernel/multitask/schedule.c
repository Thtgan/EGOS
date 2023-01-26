#include<multitask/schedule.h>

#include<blowup.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<multitask/process.h>
#include<multitask/spinlock.h>
#include<structs/singlyLinkedList.h>

SinglyLinkedList statusQueueHead[PROCESS_STATUS_NUM];
SinglyLinkedListNode* statusQueueTail[PROCESS_STATUS_NUM];

Spinlock _queueLock = SPINLOCK_UNLOCKED;

/**
 * @brief Remove process from the queue holds the process
 * 
 * @param process Process to remove
 * @return Is the process is deleted successfully
 */
static bool __removeProcessFromQueue(Process* process);

void schedule(ProcessStatus newStatus) {
    Process* current = getCurrentProcess(), * next = getStatusQueueHead(PROCESS_STATUS_READY);

    if (next != NULL) {
        setProcessStatus(current, newStatus);
        setProcessStatus(next, PROCESS_STATUS_RUNNING);

        if (current != next) {
            switchProcess(current, next);
        }
    }
}

void initSchedule() {
    for (int i = 0; i < PROCESS_STATUS_NUM; ++i) {
        initSinglyLinkedList(&statusQueueHead[i]);
        statusQueueTail[i] = &statusQueueHead[i];
    }
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
    
    initSinglyLinkedListNode(&process->node);
    singlyLinkedListInsertNext(statusQueueTail[status], &process->node);

    statusQueueTail[status] = &process->node;

    spinlockUnlock(&_queueLock);
}

Process* getStatusQueueHead(ProcessStatus status) {
    spinlockLock(&_queueLock);

    bool empty = isSinglyListEmpty(&statusQueueHead[status]);
    Process* ret = empty ? NULL : HOST_POINTER(statusQueueHead[status].next, Process, node);

    spinlockUnlock(&_queueLock);

    return ret;
}

static bool __removeProcessFromQueue(Process* process) {
    ProcessStatus status = process->status;

    void* nodeAddr = &process->node;
    for (SinglyLinkedListNode* node = &statusQueueHead[status]; node->next != &statusQueueHead[status]; node = node->next) {
        void* next = node->next;
        if (next == nodeAddr) {
            singlyLinkedListDeleteNext(node);

            if (next == statusQueueTail[status]) {
                statusQueueTail[status] = node;
            }

            initSinglyLinkedListNode(&process->node);

            return true;
        }
    }

    return false;
}