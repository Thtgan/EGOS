#if !defined(__MULTITASK_WAIT_H)
#define __MULTITASK_WAIT_H

typedef struct Wait Wait;
typedef struct WaitOperations WaitOperations;

#include<kit/types.h>
#include<multitask/schedule.h>
#include<multitask/thread.h>
#include<structs/linkedList.h>

typedef struct Wait {
    WaitOperations* operations;
    LinkedList waitList;
} Wait;

typedef struct WaitOperations {
    bool (*requestWait)(Wait* wait);
    bool (*wait)(Wait* wait, Thread* thread);
    void (*quitWaitting)(Wait* wait, Thread* thread);
} WaitOperations;

static inline void wait_initStruct(Wait* wait, WaitOperations* operations) {
    wait->operations = operations;
    linkedList_initStruct(&wait->waitList);
}

static inline bool wait_rawRequestWait(Wait* wait) {
    return wait->operations->requestWait(wait);
}

static inline bool wait_rawWait(Wait* wait, Thread* thread) {
    return wait->operations->wait(wait, thread);
}

static inline void wait_rawQuitWaitting(Wait* wait, Thread* thread) {
    wait->operations->quitWaitting(wait, thread);
}

// static inline bool wait_tryWait(Wait* wait) {
//     if (wait_rawRequestWait(wait)) {
//         Thread* currentThread = schedule_getCurrentThread();
//         thread_forceSleep(currentThread, wait);

//         return true;
//     }

//     return false;
// }

#endif // __MULTITASK_WAIT_H
