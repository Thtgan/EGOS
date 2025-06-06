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
#include<multitask/state.h>
#include<multitask/wait.h>
#include<structs/linkedList.h>
#include<structs/queue.h>
#include<system/pageTable.h>

__attribute__((naked))
static void __thread_switchContext(Thread* from, Thread* to);

void thread_initStruct(Thread* thread, Uint16 tid, Process* process, ThreadEntryPoint entry, Range* kernelStack) {
    thread->process = process;
    thread->tid = tid;
    thread->state = STATE_RUNNING;

    memory_memset(&thread->context, 0, sizeof(Context));
    
    if (kernelStack == NULL) {
        Size stackFrameNum = DIVIDE_ROUND_UP(THREAD_DEFAULT_KERNEL_STACK_SIZE, PAGE_SIZE);
        void* stackFrames = memory_allocateFrames(stackFrameNum);
        if (stackFrames == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        memory_memset(PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(stackFrames), 0, THREAD_DEFAULT_KERNEL_STACK_SIZE);

        void* kernelStackBottom = PAGING_CONVERT_HEAP_ADDRESS_P2V(stackFrames);

        extendedPageTableRoot_draw(
            process->extendedTable,
            kernelStackBottom,
            stackFrames,
            stackFrameNum,
            extraPageTableContext_getDefaultPreset(process->extendedTable->context, MEMORY_DEFAULT_PRESETS_TYPE_COW),
            EMPTY_FLAGS
        );
        ERROR_GOTO_IF_ERROR(0);
        
        thread->kernelStack = (Range) {
            .begin = (Uintptr)kernelStackBottom,
            .length = THREAD_DEFAULT_KERNEL_STACK_SIZE
        };
    } else {
        thread->kernelStack = *kernelStack;
    }

    thread->userStack = (Range) {
        .begin = (Uintptr)NULL,
        .length = 0
    };

    thread->context.rip = (Uintptr)entry;
    thread->context.rsp = thread->kernelStack.begin + thread->kernelStack.length - sizeof(Registers);

    DEBUG_ASSERT_SILENT(thread->kernelStack.length >= sizeof(Registers));
    thread->registers = (Registers*)thread->context.rsp;

    thread->remainTick = THREAD_TICK;

    linkedListNode_initStruct(&thread->processNode);
    linkedListNode_initStruct(&thread->scheduleNode);
    linkedListNode_initStruct(&thread->scheduleRunningNode);

    linkedListNode_initStruct(&thread->waitNode);

    refCounter_initStruct(&thread->refCounter);

    thread->waittingFor = NULL;

    thread->userExitStackTop = NULL;
    thread->dead = false;
    thread->isStackFromOutside = (kernelStack != NULL);
    thread->isThreadActive = false;

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
    if (!thread->isStackFromOutside) {
        void* stackFrames = PAGING_CONVERT_HEAP_ADDRESS_V2P((void*)thread->kernelStack.begin);
        Size stackFrameNum = DIVIDE_ROUND_UP(thread->kernelStack.length, PAGE_SIZE);

        extendedPageTableRoot_erase(
            process->extendedTable, (void*)thread->kernelStack.begin, stackFrameNum
        );

        memory_freeFrames(stackFrames);
    }
}

void thread_clone(Thread* thread, Thread* cloneFrom, Uint16 tid, Process* newProcess) {
    Thread* currentThread = schedule_getCurrentThread();
    if (cloneFrom != currentThread) {
        schedule_enterCritical();
    }

    thread->process = newProcess;

    thread->tid = tid;
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

    thread->remainTick = THREAD_TICK;

    thread->userExitStackTop = cloneFrom->userExitStackTop;

    linkedListNode_initStruct(&thread->processNode);
    linkedListNode_initStruct(&thread->scheduleNode);
    linkedListNode_initStruct(&thread->scheduleRunningNode);

    linkedListNode_initStruct(&thread->waitNode);

    refCounter_initStruct(&thread->refCounter);

    thread->waittingFor = NULL;

    thread->dead = false;
    thread->isStackFromOutside = false;
    thread->isThreadActive = false;

    if (cloneFrom == currentThread) {
        REGISTERS_SAVE();

        extern void* __thread_cloneReturn;
        thread->context.rip = (Uint64)&__thread_cloneReturn;
        thread->context.rsp = readRegister_RSP_64();
        thread->registers = (Registers*)thread->context.rsp;

        ExtendedPageTableRoot* newTable = extendedPageTableRoot_copyTable(cloneFrom->process->extendedTable);   //TODO: Ugly solution
        DEBUG_ASSERT_SILENT(newTable != NULL);
        newProcess->extendedTable = newTable;
        
        asm volatile("__thread_cloneReturn:");
        
        REGISTERS_RESTORE();
        ERROR_GOTO_IF_ERROR(0);
    } else {
        thread->context.rip = cloneFrom->context.rip;
        thread->context.rsp = cloneFrom->context.rsp;
        thread->registers = cloneFrom->registers;

        ExtendedPageTableRoot* newTable = extendedPageTableRoot_copyTable(cloneFrom->process->extendedTable);   //TODO: Ugly solution
        DEBUG_ASSERT_SILENT(newTable != NULL);
        newProcess->extendedTable = newTable;
    }

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
    debug_blowup("Thread dead is still running!\n");
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

    if (lastThread->dead) {
        process_notifyThreadDead(lastThread->process, lastThread);
    }
}

void thread_setupForUserProgram(Thread* thread) {
    ExtendedPageTableRoot* extendedTableRoot = thread->process->extendedTable;
    Size stackFrameNum = DIVIDE_ROUND_UP(THREAD_DEFAULT_USER_STACK_SIZE, PAGE_SIZE);
    void* stackFrames = memory_allocateFrames(stackFrameNum);
    if (stackFrames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memset(PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(stackFrames), 0, THREAD_DEFAULT_USER_STACK_SIZE);

    void* userStackBottom = PAGING_CONVERT_HEAP_ADDRESS_P2V(stackFrames);

    extendedPageTableRoot_draw(
        extendedTableRoot,
        userStackBottom,
        stackFrames,
        stackFrameNum,
        extraPageTableContext_getDefaultPreset(extendedTableRoot->context, MEMORY_DEFAULT_PRESETS_TYPE_USER_DATA),
        EMPTY_FLAGS
    );

    ERROR_GOTO_IF_ERROR(0);
    
    thread->userStack = (Range) {
        .begin = (Uintptr)userStackBottom,
        .length = THREAD_DEFAULT_USER_STACK_SIZE
    };

    return;
    ERROR_FINAL_BEGIN(0);
}

void thread_stop(Thread* thread) {
    thread_refer(thread);
    schedule_threadQuitSchedule(thread);
    thread->state = STATE_STOPPED;
    thread_derefer(thread);
}

void thread_refer(Thread* thread) {
    refCounter_refer(&thread->refCounter);
}

void thread_derefer(Thread* thread) {
    DEBUG_ASSERT_SILENT(!refCounter_check(&thread->refCounter, 0));
    if (refCounter_derefer(&thread->refCounter)) {
        if (thread == schedule_getCurrentThread() && thread->state == STATE_STOPPED) {
            schedule_yield();
        }
    }
}

__attribute__((naked))
static void __thread_switchContext(Thread* from, Thread* to) {
    REGISTERS_SAVE();

    from->registers = (Registers*)readRegister_RSP_64();
    
    Context* fromContext = &from->context;

    asm volatile(
        "mov %%rsp, 8(%0);"
        "lea __switch_return(%%rip), %%rax;"
        "mov %%rax, 0(%0);"
        :
        : "D"(fromContext)
        : "memory", "rax"
    );

    Uintptr toRIP = to->context.rip;
    Uintptr toRSP = to->context.rsp;

    mm_switchPageTable(to->process->extendedTable);

    asm volatile(
        "mov %1, %%rsp;"
        "pushq %0;"
        "retq;"
        "__switch_return:"
        :
        : "D"(toRIP), "S"(toRSP)
        : "memory"
    );
    
    REGISTERS_RESTORE();

    barrier();

    asm volatile("ret");
}