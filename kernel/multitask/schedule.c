#include<multitask/schedule.h>

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<memory/paging/directAccess.h>
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
    Process* current = getStatusQueueHead(PROCESS_STATUS_RUNNING), * next = getStatusQueueHead(PROCESS_STATUS_READY);

    if (next != NULL) {
        changeProcessStatus(current, PROCESS_STATUS_READY);
        changeProcessStatus(next, PROCESS_STATUS_RUNNING);

        switchProcess(current, next);
    }
}

void initSchedule() {
    for (int i = 0; i < PROCESS_STATUS_NUM; ++i) {
        initSinglyLinkedListNode(&statusQueueHead[i]);
        statusQueueTail[i] = &statusQueueHead[i];
    }
}

void changeProcessStatus(Process* process, ProcessStatus status) {
    Process* processDirectAccess = PA_TO_DIRECT_ACCESS_VA(process);
    if (processDirectAccess->status != PROCESS_STATUS_UNKNOWN) {
        if (!__removeProcessFromQueue(process)) {

        }  //Make sure only one same process in alll queues
    }

    processDirectAccess->status = status;
    
    singlyLinkedListInsertNext(statusQueueTail[status], &processDirectAccess->node);

    statusQueueTail[status] = &processDirectAccess->node;
}

Process* getStatusQueueHead(ProcessStatus status) {
    if (statusQueueHead[status].next == NULL) {
        return NULL;
    }
    return DIRECT_ACCESS_VA_TO_PA(HOST_POINTER(statusQueueHead[status].next, Process, node));
}

static bool __removeProcessFromQueue(Process* process) {
    Process* processDirectAccess = PA_TO_DIRECT_ACCESS_VA(process);
    ProcessStatus status = processDirectAccess->status;

    void* nodeAddr = &processDirectAccess->node;
    for (SinglyLinkedListNode* node = &statusQueueHead[status]; node->next != NULL; node = node->next) {
        if (node->next == nodeAddr) {
            singlyLinkedListDeleteNext(node);

            if (statusQueueHead[status].next == NULL) {
                statusQueueTail[status] = &statusQueueHead[status];
            }

            initSinglyLinkedListNode(&processDirectAccess->node);

            return true;
        }
    }

    return false;
}