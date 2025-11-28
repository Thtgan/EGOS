#include<multitask/reaper.h>

#include<kit/util.h>
#include<multitask/locks/semaphore.h>
#include<multitask/locks/spinlock.h>
#include<multitask/process.h>
#include<multitask/thread.h>
#include<structs/queue.h>

typedef struct __ThreadReaper {
    Semaphore threadSema;
    Spinlock queueLock;
    Queue threadQueue;
} __ThreadReaper;

void __reaper_init(__ThreadReaper* reaper);

void __reaper_submitThread(__ThreadReaper* reaper, Thread* thread);

Thread* __reaper_takeThread(__ThreadReaper* reaper);

static __ThreadReaper _reaper;

void reaper_init() {
    __reaper_init(&_reaper);
}

void reaper_daemon() {
    idt_enableInterrupt();
    while (true) {
        Thread* thread = __reaper_takeThread(&_reaper);
        process_notifyThreadDead(thread->process, thread);
        thread_clearStruct(thread);
    }
}

void reaper_submitThread(Thread* thread) {
    __reaper_submitThread(&_reaper, thread);
}

void __reaper_init(__ThreadReaper* reaper) {
    semaphore_initStruct(&reaper->threadSema, 0);
    reaper->queueLock = SPINLOCK_UNLOCKED;
    queue_initStruct(&reaper->threadQueue);
}

void __reaper_submitThread(__ThreadReaper* reaper, Thread* thread) {
    Queue* q = &reaper->threadQueue;
    Spinlock* lock = &reaper->queueLock;

    spinlock_lock(lock);
    queue_push(&reaper->threadQueue, &thread->reapNode);
    spinlock_unlock(lock);
    
    semaphore_up(&reaper->threadSema);
}

Thread* __reaper_takeThread(__ThreadReaper* reaper) {
    semaphore_down(&reaper->threadSema);

    Queue* q = &reaper->threadQueue;
    Spinlock* lock = &reaper->queueLock;

    spinlock_lock(lock);
    DEBUG_ASSERT_SILENT(!queue_isEmpty(q));
    QueueNode* node = queue_peek(q);
    queue_pop(q);
    spinlock_unlock(lock);

    return HOST_POINTER(node, Thread, reapNode);
}