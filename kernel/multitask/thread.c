#include<multitask/thread.h>

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<multitask/locks/mutex.h>
#include<multitask/process.h>
#include<multitask/reaper.h>
#include<multitask/schedule.h>
#include<multitask/signal.h>
#include<multitask/state.h>
#include<multitask/threadStack.h>
#include<multitask/wait.h>
#include<real/flags/eflags.h>
#include<structs/linkedList.h>
#include<structs/queue.h>
#include<structs/refCounter.h>
#include<system/GDT.h>
#include<system/pageTable.h>
#include<error.h>

__attribute__((naked))
static void __thread_switchContext(Thread* from, Thread* to);

static void __thread_setupKernelContext(Thread* thread, ThreadEntryPoint entry);

void thread_initStruct(Thread* thread, Uint16 tid, Process* process) {
    thread->process = process;
    thread->tid = tid;
    thread->state = STATE_RUNNING;

    memory_memset(&thread->context, 0, sizeof(Context));

    threadStack_initStruct(&thread->kernelStack, THREAD_DEFAULT_KERNEL_STACK_SIZE, process->extendedTable, DEFAULT_MEMORY_OPERATIONS_TYPE_COW, false);
    threadStack_initStruct(&thread->userStack, THREAD_DEFAULT_USER_STACK_SIZE, process->extendedTable, DEFAULT_MEMORY_OPERATIONS_TYPE_COW, true);

    thread->remainTick = THREAD_TICK;

    linkedListNode_initStruct(&thread->processNode);
    linkedListNode_initStruct(&thread->scheduleNode);
    linkedListNode_initStruct(&thread->scheduleRunningNode);
    queueNode_initStruct(&thread->reapNode);

    linkedListNode_initStruct(&thread->waitNode);

    REF_COUNTER_INIT(thread->refCounter, 0);
    thread->lock = SPINLOCK_UNLOCKED;

    thread->waittingFor = NULL;

    thread->userExitStackTop = NULL;
    thread->dead = false;
    thread->isThreadActive = false;
}

void thread_initFirstThread(Thread* thread, Uint16 tid, Process* process, void* stackBottom, Size stackSize){
    thread_initStruct(thread, tid, process);

    threadStack_initStructFromExisting(&thread->kernelStack, stackBottom, stackSize, thread->process->extendedTable, DEFAULT_MEMORY_OPERATIONS_TYPE_COW, false);
}

void thread_initNewThread(Thread* thread, Uint16 tid, Process* process, ThreadEntryPoint entry) {
    thread_initStruct(thread, tid, process);

    threadStack_touch(&thread->kernelStack);
    ERROR_GOTO_IF_ERROR(0);

    __thread_setupKernelContext(thread, entry);

    signalQueue_initStruct(&thread->signalQueue);

    return;
    ERROR_FINAL_BEGIN(0);
}

void thread_clearStruct(Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->tid != 1);
    
    if (thread->userStack.stackBottom != NULL) {
        threadStack_clearStruct(&thread->userStack);
        ERROR_GOTO_IF_ERROR(0);
    }

    threadStack_clearStruct(&thread->kernelStack);

    return;
    ERROR_FINAL_BEGIN(0);
}

void thread_clone(Thread* thread, Thread* cloneFrom, Uint16 tid, Process* newProcess, Context* retContext) {
    thread_lock(cloneFrom);

    thread_initStruct(thread, tid, newProcess);

    thread->state = cloneFrom->state;

    threadStack_initStructFromExisting(
        &thread->kernelStack,
        cloneFrom->kernelStack.stackBottom,
        cloneFrom->kernelStack.size,
        newProcess->extendedTable,
        cloneFrom->kernelStack.type,
        false
    );

    threadStack_initStructFromExisting(
        &thread->userStack,
        cloneFrom->userStack.stackBottom,
        cloneFrom->userStack.size,
        newProcess->extendedTable,
        cloneFrom->userStack.type,
        true
    );

    if (cloneFrom == schedule_getCurrentThread()) {
        DEBUG_ASSERT_SILENT(retContext != NULL);
        thread->context = retContext;
    } else {
        thread->context = cloneFrom->context;
    }

    thread->userExitStackTop = cloneFrom->userExitStackTop;

    thread_unlock(cloneFrom);

    return;
    ERROR_FINAL_BEGIN(0);
}

void thread_die(Thread* thread) {
    thread_lock(thread);
    thread->dead = true;
    thread_stop(thread);
    thread_unlock(thread);
}

void thread_sleep(Thread* thread, Wait* wait) {
    thread_lock(thread);

    DEBUG_ASSERT_SILENT(thread->state == STATE_RUNNING);
    DEBUG_ASSERT_SILENT(!idt_isInISR());
    
    do {
        schedule_threadQuitSchedule(thread);
        thread->state = STATE_SLEEP;
        thread->waittingFor = wait;
        
        thread_unlock(thread);
        wait_rawWait(wait, thread); //TODO: Split operations before actual wait(Adding to wait queue), and move before unlock
        thread_lock(thread);
    } while (wait_rawShouldWait(wait, thread));
    
    //Thread should rejoined scheduling here (thread_wakeup or forceWakeup called)
    DEBUG_ASSERT_SILENT(thread->state == STATE_RUNNING);
    thread_unlock(thread);
}

void thread_wakeup(Thread* thread) {
    thread_lock(thread);

    DEBUG_ASSERT_SILENT(thread->state == STATE_SLEEP);
    
    thread->state = STATE_RUNNING;
    thread->waittingFor = NULL;
    schedule_threadJoinSchedule(thread);

    thread_unlock(thread);
}

void thread_forceWakeup(Thread* thread) {
    thread_lock(thread);

    DEBUG_ASSERT_SILENT(thread->state == STATE_SLEEP);
    
    wait_rawQuitWaitting(thread->waittingFor, thread);
    thread_wakeup(thread);

    thread_unlock(thread);
}

void thread_switch(Thread* currentThread, Thread* nextThread) {
    DEBUG_ASSERT_SILENT(!idt_isInISR());
    static Thread* lastThread = NULL;

    lastThread = currentThread;

    __thread_switchContext(currentThread, nextThread);

    currentThread->process->lastActiveThread = currentThread;
}

void thread_stop(Thread* thread) {  //TODO: Not designed for SMP
    thread_lock(thread);
    schedule_threadQuitSchedule(thread);
    thread->state = STATE_STOPPED;
    thread_unlock(thread);
}

void thread_continue(Thread* thread) {  //TODO: Not designed for SMP
    thread_lock(thread);
    
    DEBUG_ASSERT_SILENT(thread->state != STATE_RUNNING);
    
    thread->state = STATE_RUNNING;
    thread->waittingFor = NULL;
    schedule_threadJoinSchedule(thread);

    thread_unlock(thread);
}

void thread_lock(Thread* thread) {
    if (REF_COUNTER_CHECK(thread->refCounter, 0)) {
        schedule_enterCritical();
        spinlock_lock(&thread->lock);
    }
    REF_COUNTER_REFER(thread->refCounter);
}

void thread_unlock(Thread* thread) {
    DEBUG_ASSERT_SILENT(!REF_COUNTER_CHECK(thread->refCounter, 0));
    DEBUG_ASSERT_SILENT(spinlock_isLocked(&thread->lock));
    if (REF_COUNTER_DEREFER(thread->refCounter) != 0) {
        return;
    }

    spinlock_unlock(&thread->lock);
    schedule_leaveCritical();

    bool dead = thread->dead, isCurrent = (thread == schedule_getCurrentThread()), stopped = (thread->state == STATE_STOPPED);

    if (thread->dead) {
        DEBUG_ASSERT_SILENT(stopped);
        reaper_submitThread(thread);
        if (isCurrent) {
            schedule_yield();
            debug_blowup("Cleared thread still running\n");
        }
    }
    
    if (!isCurrent) {
        return;
    }

    if (stopped) {
        schedule_yield();
    }

    thread_handleSignalIfAny(thread);
}

void thread_signal(Thread* thread, int signal) {
    if (thread == schedule_getCurrentThread()) {
        thread_handleSignal(thread, signal);
    } else {
        signalQueue_pend(&thread->signalQueue, signal);
        //TODO: Interrupt thread from interruptable sleeping
    }
}

void thread_handleSignal(Thread* thread, int signal) {
    //TODO: Add masking support
    DEBUG_ASSERT_SILENT(
        signal != SIGNAL_SIGKILL &&
        signal != SIGNAL_SIGSTOP &&
        signal != SIGNAL_SIGTSTP
    );

    DEBUG_ASSERT_SILENT(thread == schedule_getCurrentThread());
    
    SignalHandler handler = thread->process->signalHandlers[signal];
    if (handler == SIGACTION_HANDLER_DFL) {
        SignalDefaultHandler defaultHandler = signal_defaultHandlers[signal];
        handler = signalDefaultHandler_funcs[defaultHandler];
    }
    handler(signal);    //TODO: Should be in user mode
}

void thread_handleSignalIfAny(Thread* thread) {
    int pendingSignal = signalQueue_getPending(&thread->signalQueue);
    if (pendingSignal == 0) {
        return;
    }
    thread_handleSignal(thread, pendingSignal);
}

__attribute__((naked))
static void __thread_switchContext(Thread* from, Thread* to) {
    CONTEXT_SAVE(__switch_return);
    from->context = (Context*)readRegister_RSP_64();

    mm_switchPageTable(to->process->extendedTable);

    writeRegister_RSP_64((Uintptr)to->context);
    CONTEXT_RESTORE(__switch_return);
}

static void __thread_setupKernelContext(Thread* thread, ThreadEntryPoint entry) {
    Context* context = (Context*)(threadStack_getStackTop(&thread->kernelStack) - sizeof(Context));
    
    void* pContext = paging_fastTranslate(thread->kernelStack.extendedTable, context);
    Context* contextWrite = (Context*)PAGING_CONVERT_KERNEL_MEMORY_P2V(pContext);

    contextWrite->rip = (Uintptr)entry;
    Registers* regs = &contextWrite->regs;

    memory_memset(regs, 0, sizeof(regs));
    regs->cs = SEGMENT_KERNEL_CODE;
    regs->ds = SEGMENT_KERNEL_DATA;
    regs->ss = SEGMENT_KERNEL_DATA;
    regs->es = SEGMENT_KERNEL_DATA;
    regs->fs = SEGMENT_KERNEL_DATA;
    regs->gs = SEGMENT_KERNEL_DATA;
    regs->eflags = EFLAGS_FIXED;

    thread->context = context;
}