#include<multitask/thread.h>

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<multitask/signal.h>
#include<multitask/state.h>
#include<multitask/wait.h>
#include<real/flags/eflags.h>
#include<structs/linkedList.h>
#include<structs/queue.h>
#include<system/GDT.h>
#include<system/pageTable.h>

__attribute__((naked))
static void __thread_switchContext(Thread* from, Thread* to);

static void __thread_allocateStackStack(Range* stack, ExtendedPageTableRoot* extendedTable, MemoryDefaultPresetType memoryType, Size size);

static void __thread_setupKernelContext(Thread* thread, ThreadEntryPoint entry);

void thread_initStruct(Thread* thread, Uint16 tid, Process* process) {
    thread->process = process;
    thread->tid = tid;
    thread->state = STATE_RUNNING;

    memory_memset(&thread->context, 0, sizeof(Context));

    thread->kernelStack = (Range) {
        .begin = (Uintptr)NULL,
        .length = 0
    };

    thread->userStack = (Range) {
        .begin = (Uintptr)NULL,
        .length = 0
    };

    thread->remainTick = THREAD_TICK;

    linkedListNode_initStruct(&thread->processNode);
    linkedListNode_initStruct(&thread->scheduleNode);
    linkedListNode_initStruct(&thread->scheduleRunningNode);

    linkedListNode_initStruct(&thread->waitNode);

    refCounter_initStruct(&thread->refCounter);

    thread->waittingFor = NULL;

    thread->userExitStackTop = NULL;
    thread->dead = false;
    thread->isThreadActive = false;
}

void thread_initFirstThread(Thread* thread, Uint16 tid, Process* process, Range* kernelStack) {
    thread_initStruct(thread, tid, process);

    thread->kernelStack = *kernelStack;
}

void thread_initNewThread(Thread* thread, Uint16 tid, Process* process, ThreadEntryPoint entry) {
    thread_initStruct(thread, tid, process);

    __thread_allocateStackStack(
        &thread->kernelStack,
        process->extendedTable,
        MEMORY_DEFAULT_PRESETS_TYPE_COW,
        THREAD_DEFAULT_KERNEL_STACK_SIZE
    );
    ERROR_GOTO_IF_ERROR(0);

    __thread_setupKernelContext(thread, entry);

    signalQueue_initStruct(&thread->signalQueue);

    return;
    ERROR_FINAL_BEGIN(0);
}

void thread_clearStruct(Thread* thread) {
    Process* process = thread->process;
    if ((void*)thread->userStack.begin != NULL) {
        void* stackFrames = PAGING_CONVERT_HEAP_ADDRESS_V2P((void*)thread->userStack.begin);
        Size stackFrameNum = DIVIDE_ROUND_UP(thread->userStack.length, PAGE_SIZE);

        extendedPageTableRoot_erase(
            process->extendedTable, (void*)thread->userStack.begin, stackFrameNum
        );

        memory_freeFrames(stackFrames);
    }

    DEBUG_ASSERT_SILENT((void*)thread->kernelStack.begin != NULL);
    if (thread->tid != 1) {
        void* stackFrames = PAGING_CONVERT_HEAP_ADDRESS_V2P((void*)thread->kernelStack.begin);
        Size stackFrameNum = DIVIDE_ROUND_UP(thread->kernelStack.length, PAGE_SIZE);

        extendedPageTableRoot_erase(
            process->extendedTable, (void*)thread->kernelStack.begin, stackFrameNum
        );

        memory_freeFrames(stackFrames);
    }
}

void thread_clone(Thread* thread, Thread* cloneFrom, Uint16 tid, Process* newProcess, Context* retContext) {
    Thread* currentThread = schedule_getCurrentThread();
    if (cloneFrom != currentThread) {
        schedule_enterCritical();
    }

    thread_initStruct(thread, tid, newProcess);

    thread->state = cloneFrom->state;

    if (cloneFrom->userStack.begin != 0) {
        thread_setupForUserProgram(thread);
        ERROR_GOTO_IF_ERROR(0);
        memory_memcpy((void*)thread->userStack.begin, (void*)cloneFrom->userStack.begin, cloneFrom->userStack.length);
    } else {
        thread->userStack = (Range) {
            .begin = 0,
            .length = 0
        };
    }

    thread->kernelStack = cloneFrom->kernelStack;

    if (cloneFrom == currentThread) {
        DEBUG_ASSERT_SILENT(retContext != NULL);
        thread->context = retContext;
    } else {
        thread->context = cloneFrom->context;
    }

    thread->userExitStackTop = cloneFrom->userExitStackTop;

    if (cloneFrom != currentThread) {
        schedule_leaveCritical();
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void thread_die(Thread* thread) {
    thread_refer(thread);
    thread->dead = true;
    thread_stop(thread);
    thread_derefer(thread);

    // if (thread == schedule_getCurrentThread()) {    //If thread is current one, it should not reach here
    //     debug_blowup("Dead thread is still running!\n");
    // }
}

bool thread_trySleep(Thread* thread, Wait* wait) {
    if (!wait_rawRequestWait(wait)) {
        return false;
    }

    thread_forceSleep(thread, wait);

    return true;
}

void thread_forceSleep(Thread* thread, Wait* wait) {
    DEBUG_ASSERT_SILENT(thread->state == STATE_RUNNING);
    DEBUG_ASSERT_SILENT(!idt_isInISR());
    
    schedule_threadQuitSchedule(thread);
    
    thread->state = STATE_SLEEP;
    thread->waittingFor = wait;
    
    wait_rawWait(wait, thread);
    
    //Thread should rejoined scheduling here (thread_wakeup or forceWakeup called)
    DEBUG_ASSERT_SILENT(thread->state == STATE_RUNNING);
}

void thread_wakeup(Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->state == STATE_SLEEP);
    
    thread->state = STATE_RUNNING;
    thread->waittingFor = NULL;

    schedule_threadJoinSchedule(thread);
}

void thread_forceWakeup(Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->state == STATE_SLEEP);
    
    wait_rawQuitWaitting(thread->waittingFor, thread);
    
    thread_wakeup(thread);
}

void thread_switch(Thread* currentThread, Thread* nextThread) {
    DEBUG_ASSERT_SILENT(!idt_isInISR());
    static Thread* lastThread = NULL;

    lastThread = currentThread;

    __thread_switchContext(currentThread, nextThread);

    currentThread->process->lastActiveThread = currentThread;

    // if (lastThread->dead) {
    //     process_notifyThreadDead(lastThread->process, lastThread);
    //     thread_clearStruct(thread); //TODO: Clear thread somewhere
    // }
}

void thread_setupForUserProgram(Thread* thread) {
    __thread_allocateStackStack(
        &thread->userStack,
        thread->process->extendedTable,
        MEMORY_DEFAULT_PRESETS_TYPE_USER_DATA,
        THREAD_DEFAULT_USER_STACK_SIZE
    );
}

void thread_stop(Thread* thread) {  //TODO: Not designed for SMP
    thread_refer(thread);
    schedule_threadQuitSchedule(thread);
    thread->state = STATE_STOPPED;
    thread_derefer(thread);
}

void thread_continue(Thread* thread) {  //TODO: Not designed for SMP
    DEBUG_ASSERT_SILENT(thread->state != STATE_RUNNING);
    
    thread->state = STATE_RUNNING;
    thread->waittingFor = NULL;

    schedule_threadJoinSchedule(thread);
}

void thread_refer(Thread* thread) {
    refCounter_refer(&thread->refCounter);
}

void thread_derefer(Thread* thread) {
    DEBUG_ASSERT_SILENT(!refCounter_check(&thread->refCounter, 0));
    if (!refCounter_derefer(&thread->refCounter)) {
        return;
    }

    bool dead = thread->dead, isCurrent = (thread == schedule_getCurrentThread()), stopped = (thread->state == STATE_STOPPED);

    if (dead) {
        DEBUG_ASSERT_SILENT(stopped);
        // process_notifyThreadDead(thread->process, thread);
        // thread_clearStruct(thread); //TODO: Clear thread somewhere
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

static void __thread_allocateStackStack(Range* stack, ExtendedPageTableRoot* extendedTable, MemoryDefaultPresetType memoryType, Size size) {
    Size stackFrameNum = DIVIDE_ROUND_UP(size, PAGE_SIZE);
    void* stackFrames = memory_allocateFrames(stackFrameNum);
    if (stackFrames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memset(PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(stackFrames), 0, THREAD_DEFAULT_KERNEL_STACK_SIZE);

    void* stackBottom = PAGING_CONVERT_HEAP_ADDRESS_P2V(stackFrames);

    extendedPageTableRoot_draw(
        extendedTable,
        stackBottom,
        stackFrames,
        stackFrameNum,
        extraPageTableContext_getDefaultPreset(extendedTable->context, memoryType),
        EMPTY_FLAGS
    );
    ERROR_GOTO_IF_ERROR(0);
    
    stack->begin = (Uintptr)stackBottom;
    stack->length = size;

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __thread_setupKernelContext(Thread* thread, ThreadEntryPoint entry) {
    Range* stack = &thread->kernelStack;
    Context* context = (Context*)(stack->begin + stack->length - sizeof(Context));

    Context* contextWrite = (Context*)PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(PAGING_CONVERT_HEAP_ADDRESS_V2P(context));

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