#if !defined(__MULTITASK_THREAD_H)
#define __MULTITASK_THREAD_H

typedef struct Thread Thread;
typedef void (*ThreadEntryPoint)();

#include<kit/types.h>
#include<multitask/process.h>
#include<multitask/signal.h>
#include<multitask/state.h>
#include<multitask/threadStack.h>
#include<multitask/wait.h>
#include<structs/linkedList.h>
#include<structs/queue.h>
#include<structs/refCounter.h>

#define THREAD_DEFAULT_KERNEL_STACK_SIZE    4 * PAGE_SIZE
#define THREAD_DEFAULT_USER_STACK_SIZE      4 * PAGE_SIZE

typedef struct Thread {
    Process* process;

    Uint16 tid;
    State state;

    ThreadStack kernelStack;
    ThreadStack userStack;

    Context* context;

#define THREAD_TICK 10
    Uint16 remainTick;

    LinkedListNode processNode;
    LinkedListNode scheduleNode;
    LinkedListNode scheduleRunningNode;
    QueueNode reapNode;

    RefCounter refCounter;
    Spinlock lock;

    Wait* waittingFor;
    LinkedListNode waitNode;

    void* userExitStackTop; //TODO: Remove this

    bool dead;
    bool isThreadActive;

    SignalQueue signalQueue;
} Thread;

void thread_initStruct(Thread* thread, Uint16 tid, Process* process);

void thread_initFirstThread(Thread* thread, Uint16 tid, Process* process, void* stackBottom, Size stackSize);

void thread_initNewThread(Thread* thread, Uint16 tid, Process* process, ThreadEntryPoint entry);

void thread_clearStruct(Thread* thread);

void thread_clone(Thread* thread, Thread* cloneFrom, Uint16 tid, Process* newProcess, Context* retContext);

void thread_die(Thread* thread);

void thread_sleep(Thread* thread, Wait* wait);

void thread_wakeup(Thread* thread);

void thread_forceWakeup(Thread* thread);

void thread_switch(Thread* currentThread, Thread* nextThread);

void thread_stop(Thread* thread);

void thread_continue(Thread* thread);

void thread_lock(Thread* thread);

void thread_unlock(Thread* thread);

void thread_signal(Thread* thread, int signal);

void thread_handleSignal(Thread* thread, int signal);

void thread_handleSignalIfAny(Thread* thread);

#endif // __MULTITASK_THREAD_H
