#include<multitask/schedule.h>

#include<blowup.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<multitask/process.h>
#include<structs/singlyLinkedList.h>

SinglyLinkedList statusQueueHead[PROCESS_STATUS_NUM];
SinglyLinkedListNode* statusQueueTail[PROCESS_STATUS_NUM];

/**
 * @brief Remove process from the queue holds the process
 * 
 * @param process Process to remove
 * @return Is the process is deleted successfully
 */
static bool __removeProcessFromQueue(Process* process);

void schedule() {
    Process* current = getCurrentProcess(), * next = getStatusQueueHead(PROCESS_STATUS_READY);
    if (current->status != PROCESS_STATUS_RUNNING) {
        blowup("Incorrect process status\n");
    }

    if (next != NULL) {
        changeProcessStatus(current, PROCESS_STATUS_READY);
        changeProcessStatus(next, PROCESS_STATUS_RUNNING);

        switchProcess(current, next);
    }
}

void initSchedule() {
    for (int i = 0; i < PROCESS_STATUS_NUM; ++i) {
        initSinglyLinkedList(&statusQueueHead[i]);
        statusQueueTail[i] = &statusQueueHead[i];
    }
}

void changeProcessStatus(Process* process, ProcessStatus status) {
    if (process->status == status) {
        return;
    }

    if (process->status != PROCESS_STATUS_UNKNOWN && !__removeProcessFromQueue(process)) {
        blowup("Remove process from queue failed\n");
    }

    process->status = status;
    
    initSinglyLinkedListNode(&process->node);
    singlyLinkedListInsertNext(statusQueueTail[status], &process->node);

    statusQueueTail[status] = &process->node;
}

Process* getStatusQueueHead(ProcessStatus status) {
    if (isSinglyListEmpty(&statusQueueHead[status])) {
        return NULL;
    }
    return HOST_POINTER(statusQueueHead[status].next, Process, node);
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