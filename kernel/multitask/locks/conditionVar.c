#include<multitask/locks/conditionVar.h>

#include<multitask/wait.h>
#include<multitask/schedule.h>
#include<multitask/thread.h>
#include<structs/linkedList.h>

#include<uart.h>

static bool __conditionVar_waitOperations_tryTake(Wait* wait, Thread* thread);

static bool __conditionVar_waitOperations_shouldWait(Wait* wait, Thread* thread);

static void __conditionVar_waitOperations_wait(Wait* wait, Thread* thread);

static void __conditionVar_waitOperations_quitWaitting(Wait* wait, Thread* thread);

static WaitOperations _semaphore_waitOperations = {
    .tryTake        = __conditionVar_waitOperations_tryTake,
    .shouldWait     = __conditionVar_waitOperations_shouldWait,
    .wait           = __conditionVar_waitOperations_wait,
    .quitWaitting   = __conditionVar_waitOperations_quitWaitting
};

void conditionVar_initStruct(ConditionVar* cond) {
    wait_initStruct(&cond->wait, &_semaphore_waitOperations);
}

void conditionVar_wait(ConditionVar* cond, Mutex* lock, ConditionVariableFunc func, void* arg) {
    while (!func(arg)) {
        conditionVar_waitOnce(cond, lock);
    }
}

void conditionVar_waitOnce(ConditionVar* cond, Mutex* lock) {
    Thread* currentThread = schedule_getCurrentThread();
    
    DEBUG_ASSERT_SILENT(mutex_isLocked(lock) && lock->acquiredBy == currentThread);
    
    schedule_enterCritical();
    
    mutex_release(lock);

    thread_sleep(currentThread, &cond->wait);

    mutex_acquire(lock);
}

void conditionVar_notify(ConditionVar* cond) {
    Wait* wait = &cond->wait;
    if (!linkedList_isEmpty(&wait->waitList)) {
        Thread* thread = HOST_POINTER(linkedListNode_getNext(&wait->waitList), Thread, waitNode);
        linkedListNode_delete(&thread->waitNode);
        thread_wakeup(thread);
    }
}

void conditionVar_notifyAll(ConditionVar* cond) {
    Wait* wait = &cond->wait;
    while (!linkedList_isEmpty(&wait->waitList)) {
        Thread* thread = HOST_POINTER(linkedListNode_getNext(&wait->waitList), Thread, waitNode);
        linkedListNode_delete(&thread->waitNode);
        thread_wakeup(thread);
    }
}

static bool __conditionVar_waitOperations_tryTake(Wait* wait, Thread* thread) {
    return true;
}

static bool __conditionVar_waitOperations_shouldWait(Wait* wait, Thread* thread) {
    return false;
}

static void __conditionVar_waitOperations_wait(Wait* wait, Thread* thread) {
    DEBUG_ASSERT_SILENT(thread == schedule_getCurrentThread());
    DEBUG_ASSERT_SILENT(thread->waittingFor == wait);
    
    linkedListNode_insertFront(&wait->waitList, &thread->waitNode);

    if (schedule_isInCritical()) {
        schedule_leaveCritical();
        DEBUG_ASSERT_SILENT(!schedule_isInCritical());
    }

    schedule_yield();
}

static void __conditionVar_waitOperations_quitWaitting(Wait* wait, Thread* thread) {
    if (thread->waittingFor != NULL) {
        DEBUG_ASSERT_SILENT(thread->waittingFor == wait);
        linkedListNode_delete(&thread->waitNode);
    }
}