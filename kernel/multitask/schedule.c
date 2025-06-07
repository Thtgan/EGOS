#include<multitask/schedule.h>

#include<kit/atomic.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<multitask/locks/mutex.h>
#include<multitask/locks/spinlock.h>
#include<multitask/process.h>
#include<multitask/thread.h>
#include<real/simpleAsmLines.h>
#include<real/flags/eflags.h>
#include<structs/linkedList.h>
#include<structs/bitmap.h>
#include<debug.h>
#include<error.h>

static Process* _schedule_rootProcess;   //Takes all threads not visible for user
static Process* _schedule_initProcess;

static Mutex _schedule_lock;
static Mutex _schedule_queueLock;

static bool _schedule_started = false;  //TODO: Remove this?
static Uint32 _schedule_criticalCount = 0;
static bool _schedule_delayingYield = false;

static LinkedList _schedule_processes;
static LinkedList _schedule_threads;
static LinkedList _schedule_runningThreads;
static Thread* _schedule_currentThread = NULL;

static Uint16 _schedule_lastAllocatedID = 0;
static Bitmap _schedule_idBitmap;
static Spinlock __schedule_idBitmapLock;
#define __SCHEDULE_MAXIMUM_ID_NUM (1u << 16)
static Uint8 _schedule_idBitmapBits[__SCHEDULE_MAXIMUM_ID_NUM / 8];

static void __schedule_doYield();

static void __schedule_idle();

static Thread* __schedule_selectNextThread();

static Thread* __schedule_initFirstThread();

static void* __schedule_earlyStackBottom;

void schedule_setEarlyStackBottom(void* stackBottom) {
    __schedule_earlyStackBottom = stackBottom;
}

void schedule_init() {
    DEBUG_ASSERT_SILENT(!_schedule_started);
    memory_memset(_schedule_idBitmapBits, 0, sizeof(_schedule_idBitmapBits));
    bitmap_initStruct(&_schedule_idBitmap, __SCHEDULE_MAXIMUM_ID_NUM, &_schedule_idBitmap);
    __schedule_idBitmapLock = SPINLOCK_UNLOCKED;

    linkedList_initStruct(&_schedule_processes);
    linkedList_initStruct(&_schedule_threads);
    linkedList_initStruct(&_schedule_runningThreads);

    mutex_initStruct(&_schedule_lock, MUTEX_FLAG_CRITICAL);
    mutex_initStruct(&_schedule_queueLock, MUTEX_FLAG_CRITICAL);
    
    _schedule_rootProcess = memory_allocate(sizeof(Process));
    if (_schedule_rootProcess == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    ExtendedPageTableRoot* newTable = extendedPageTableRoot_copyTable(mm->extendedTable);
    ERROR_GOTO_IF_ERROR(0);
    
    bitmap_setBit(&_schedule_idBitmap, 0);
    process_initStruct(_schedule_rootProcess, 0, "root", newTable);
    ERROR_GOTO_IF_ERROR(0);
    
    Thread* t = process_createThread(_schedule_rootProcess, __schedule_idle);
    ERROR_GOTO_IF_ERROR(0);
    
    _schedule_initProcess = memory_allocate(sizeof(Process));
    if (_schedule_initProcess == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    bitmap_setBit(&_schedule_idBitmap, 1);
    process_initStruct(_schedule_initProcess, 1, "init", mm->extendedTable);
    ERROR_GOTO_IF_ERROR(0);
    
    _schedule_currentThread = __schedule_initFirstThread();
    ERROR_GOTO_IF_ERROR(0);

    DEBUG_MARK_PRINT("%p-%u %p-%u\n", t, t->tid, _schedule_currentThread, _schedule_currentThread->tid);
    
    _schedule_started = true;

    schedule_addProcess(_schedule_rootProcess);
    schedule_addProcess(_schedule_initProcess);

    return;
    ERROR_FINAL_BEGIN(0);
}

Uint16 schedule_allocateNewID() {
    spinlock_lock(&__schedule_idBitmapLock);
    Index64 ret = bitmap_findFirstClear(&_schedule_idBitmap, _schedule_lastAllocatedID);
    if (ret != INVALID_INDEX64) {
        bitmap_setBit(&_schedule_idBitmap, ret);
        _schedule_lastAllocatedID = ret;
    }
    spinlock_unlock(&__schedule_idBitmapLock);
    return ret;
}

void schedule_releaseID(Uint16 id) {
    spinlock_lock(&__schedule_idBitmapLock);
    bitmap_clearBit(&_schedule_idBitmap, id);
    spinlock_unlock(&__schedule_idBitmapLock);
}

void schedule_tick() {
    if (!_schedule_started) {
        return;
    }

    if (--_schedule_currentThread->remainTick == 0) {
        _schedule_currentThread->remainTick = THREAD_TICK;
        schedule_yield();
    }
}

bool schedule_yield() {
    bool ret = idt_isInISR();
    if (ret) {
        _schedule_delayingYield = true;
    } else {
        __schedule_doYield();
    }
    return ret;
}

void schedule_isrDelayYield() {
    if (_schedule_delayingYield && !idt_isInISR()) {
        _schedule_delayingYield = false;
        __schedule_doYield();
    }
}

void schedule_addProcess(Process* process) {
    DEBUG_ASSERT_SILENT(!process->isProcessActive);
    
    mutex_acquire(&_schedule_queueLock);
    
    linkedListNode_insertBack(&_schedule_processes, &process->scheduleNode);
    for (LinkedListNode* node = linkedListNode_getNext(&process->threads); node != &process->threads; node = node->next) {
        Thread* thread = HOST_POINTER(node, Thread, processNode);
        schedule_addThread(thread);
    }

    process->isProcessActive = true;
    mutex_release(&_schedule_queueLock);
}

void schedule_removeProcess(Process* process) {
    DEBUG_ASSERT_SILENT(process->isProcessActive);

    mutex_acquire(&_schedule_queueLock);

    linkedListNode_delete(&process->scheduleNode);
    for (LinkedListNode* node = linkedListNode_getNext(&process->threads); node != &process->threads; node = node->next) {
        Thread* thread = HOST_POINTER(node, Thread, processNode);
        schedule_removeThread(thread);
    }

    process->isProcessActive = false;
    mutex_release(&_schedule_queueLock);
}

void schedule_addThread(Thread* thread) {
    DEBUG_ASSERT_SILENT(!thread->isThreadActive);

    mutex_acquire(&_schedule_queueLock);
    linkedListNode_insertBack(&_schedule_threads, &thread->scheduleNode);

    thread->isThreadActive = true;

    if (thread->state == STATE_RUNNING) {
        schedule_threadJoinSchedule(thread);
    }

    mutex_release(&_schedule_queueLock);
}

void schedule_removeThread(Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->isThreadActive);

    mutex_acquire(&_schedule_queueLock);
    if (thread->state == STATE_RUNNING) {
        schedule_threadQuitSchedule(thread);
    }

    linkedListNode_delete(&thread->scheduleNode);

    thread->isThreadActive = false;

    mutex_release(&_schedule_queueLock);
}

void schedule_threadJoinSchedule(Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->state == STATE_RUNNING);
    DEBUG_ASSERT_SILENT(thread->isThreadActive);

    mutex_acquire(&_schedule_queueLock);
    linkedListNode_insertBack(&_schedule_runningThreads, &thread->scheduleRunningNode);
    mutex_release(&_schedule_queueLock);
}

void schedule_threadQuitSchedule(Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->state == STATE_RUNNING);
    DEBUG_ASSERT_SILENT(thread->isThreadActive);

    mutex_acquire(&_schedule_queueLock);
    linkedListNode_delete(&thread->scheduleRunningNode);
    mutex_release(&_schedule_queueLock);
}

Process* schedule_getCurrentProcess() {
    DEBUG_ASSERT_SILENT(_schedule_started);
    return _schedule_currentThread->process;
}

Process* schedule_getProcessFromPID(Uint16 pid) {
    mutex_acquire(&_schedule_queueLock);
    for (LinkedListNode* node = linkedListNode_getNext(&_schedule_processes); node != &_schedule_processes; node = node->next) {
        Process* currentProcess = HOST_POINTER(node, Process, scheduleNode);
        if (currentProcess->pid == pid) {
            mutex_release(&_schedule_queueLock);
            return currentProcess;
        }
    }
    mutex_release(&_schedule_queueLock);

    ERROR_THROW_NO_GOTO(ERROR_ID_NOT_FOUND);
    return NULL;
}

Thread* schedule_getCurrentThread() {
    DEBUG_ASSERT_SILENT(_schedule_started);
    return _schedule_currentThread;
}

Thread* schedule_getThreadFromTID(Uint16 tid) {
    mutex_acquire(&_schedule_queueLock);
    for (LinkedListNode* node = linkedListNode_getNext(&_schedule_threads); node != &_schedule_threads; node = node->next) {
        Thread* currentThread = HOST_POINTER(node, Thread, scheduleNode);
        if (currentThread->tid == tid) {
            mutex_release(&_schedule_queueLock);
            return currentThread;
        }
    }
    mutex_release(&_schedule_queueLock);

    ERROR_THROW_NO_GOTO(ERROR_ID_NOT_FOUND);
    return NULL;
}

void schedule_enterCritical() {
    cli();
    ATOMIC_INC_FETCH(&_schedule_criticalCount);
}

void schedule_leaveCritical() {
    if (ATOMIC_DEC_FETCH(&_schedule_criticalCount) == 0) {
        sti();
    }
}

void schedule_yieldIfStopped() {    //TODO: Remove this?
    if (schedule_getCurrentThread()->state == STATE_STOPPED) {
        schedule_yield();
    }
}

Process* schedule_fork() {
    Process* newProcess = memory_allocate(sizeof(Process));
    if (newProcess == NULL) {
        ERROR_ASSERT_ANY();
    }

    Uint16 tid = schedule_getCurrentThread()->tid, pid = schedule_allocateNewID();
    
    Process* currentProcess = schedule_getCurrentProcess();
    process_clone(newProcess, pid, currentProcess);
    ERROR_GOTO_IF_ERROR(1);

    if (schedule_getCurrentThread()->tid == tid) {
        schedule_addProcess(newProcess);
        
        return newProcess;
    }
    
    return NULL;
    
    ERROR_FINAL_BEGIN(2);
    process_clearStruct(newProcess);
    ERROR_FINAL_BEGIN(1);
    memory_free(newProcess);
    ERROR_FINAL_BEGIN(0);
    return 0;
}

static void __schedule_doYield() {
    DEBUG_ASSERT_SILENT(!idt_isInISR());
    mutex_acquire(&_schedule_lock);

    Thread* currentThread = _schedule_currentThread, * nextThread = __schedule_selectNextThread();

    if (currentThread != nextThread) {
        mutex_release(&_schedule_lock);
        _schedule_currentThread = nextThread;   //TODO: Not safe
        
        thread_switch(currentThread, nextThread);

        mutex_acquire(&_schedule_lock);
    }

    mutex_release(&_schedule_lock);
}

static void __schedule_idle() {
    sti();
    while (true) {
        hlt();
        // scheduler_yield(schedule_getCurrentScheduler());
        //No, no yield here, currently it will make system freezed for input
    }

    debug_blowup("Idle is trying to return\n");
}

static Thread* __schedule_selectNextThread() {
    mutex_acquire(&_schedule_queueLock);

    DEBUG_ASSERT_SILENT(!linkedList_isEmpty(&_schedule_runningThreads));
    Thread* ret = HOST_POINTER(linkedListNode_getNext(&_schedule_runningThreads), Thread, scheduleRunningNode);
    linkedListNode_delete(&ret->scheduleRunningNode);
    linkedListNode_insertFront(&_schedule_runningThreads, &ret->scheduleRunningNode);

    mutex_release(&_schedule_queueLock);

    return ret;
}

void schedule_collectOrphans(Process* process) {
    for (LinkedListNode* node = linkedListNode_getNext(&process->childProcesses); node != &process->childProcesses; node = node->next) {
        Process* childProcess = HOST_POINTER(node, Process, childProcessNode);

        DEBUG_ASSERT_SILENT(childProcess->ppid = process->pid);
        process_setParent(childProcess, _schedule_initProcess);
    }
}

static Thread* __schedule_initFirstThread() {
    Thread* firstThread = memory_allocate(sizeof(Thread));
    if (firstThread == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Range initKernelStack = {
        .begin = (Uintptr)__schedule_earlyStackBottom,
        .length = THREAD_DEFAULT_KERNEL_STACK_SIZE
    };

    Uint16 tid = schedule_allocateNewID();
    thread_initFirstThread(firstThread, tid, _schedule_initProcess, &initKernelStack);
    ERROR_GOTO_IF_ERROR(0);

    process_addThread(_schedule_initProcess, firstThread);

    return firstThread;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}