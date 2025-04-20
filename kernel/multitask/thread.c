#include<multitask/thread.h>

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<multitask/state.h>
#include<multitask/wait.h>
#include<structs/linkedList.h>
#include<structs/queue.h>
#include<system/pageTable.h>

#define __THREAD_DEFAULT_KERNEL_STACK_SIZE  4 * PAGE_SIZE
#define __THREAD_DEFAULT_USER_STACK_SIZE    4 * PAGE_SIZE

void thread_initStruct(Thread* thread, Uint16 tid, Process* process, ThreadEntryPoint entry, Range* kernelStack) {
    thread->process = process;
    thread->tid = tid;
    thread->state = STATE_RUNNING;

    memory_memset(&thread->context, 0, sizeof(Context));
    
    if (kernelStack == NULL) {
        void* newKernelStack = memory_allocateFrame(DIVIDE_ROUND_UP(__THREAD_DEFAULT_KERNEL_STACK_SIZE, PAGE_SIZE));
        if (newKernelStack == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        
        thread->kernelStack = (Range) {
            .begin = (Uintptr)paging_convertAddressP2V(newKernelStack),
            .length = __THREAD_DEFAULT_KERNEL_STACK_SIZE
        };
        memory_memset((void*)thread->kernelStack.begin, 0, thread->kernelStack.length);
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
    if ((void*)thread->userStack.begin != NULL) {
        memory_freeFrame((void*)thread->userStack.begin);
    }

    DEBUG_ASSERT_SILENT((void*)thread->kernelStack.begin != NULL);
    if (!thread->isStackFromOutside) {
        memory_freeFrame((void*)thread->kernelStack.begin);
    }
}

void thread_clone(Thread* thread, Thread* cloneFrom, Uint16 tid) {
    Thread* currentThread = schedule_getCurrentThread();
    if (cloneFrom != currentThread) {
        schedule_enterCritical();
    }

    thread->process = cloneFrom->process;

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
        
        void* newKernelStack = memory_allocateFrame(DIVIDE_ROUND_UP(cloneFrom->kernelStack.length, PAGE_SIZE));
        if (newKernelStack == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        
        thread->kernelStack = (Range) {
            .begin = (Uintptr)paging_convertAddressP2V(newKernelStack),
            .length = cloneFrom->kernelStack.length
        };
        memory_memcpy((void*)thread->kernelStack.begin, (void*)cloneFrom->kernelStack.begin, cloneFrom->kernelStack.length);

        extern void* __thread_cloneReturn;
        thread->context.rip = (Uint64)&__thread_cloneReturn;
        thread->context.rsp = readRegister_RSP_64() - (cloneFrom->kernelStack.begin + cloneFrom->kernelStack.length) + (thread->kernelStack.begin + thread->kernelStack.length);
        thread->registers = (Registers*)((Uintptr)cloneFrom->registers - (cloneFrom->kernelStack.begin + cloneFrom->kernelStack.length) + (thread->kernelStack.begin + thread->kernelStack.length));

        asm volatile("__thread_cloneReturn:");

        REGISTERS_RESTORE();
    } else {
        void* newKernelStack = memory_allocateFrame(DIVIDE_ROUND_UP(cloneFrom->kernelStack.length, PAGE_SIZE));
        if (newKernelStack == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        
        thread->kernelStack = (Range) {
            .begin = (Uintptr)paging_convertAddressP2V(newKernelStack),
            .length = cloneFrom->kernelStack.length
        };
        memory_memcpy((void*)thread->kernelStack.begin, (void*)cloneFrom->kernelStack.begin, cloneFrom->kernelStack.length);

        thread->context.rip = cloneFrom->context.rip;
        thread->context.rsp = readRegister_RSP_64() - (cloneFrom->kernelStack.begin + cloneFrom->kernelStack.length) + (thread->kernelStack.begin + thread->kernelStack.length);
        thread->registers = (Registers*)((Uintptr)cloneFrom->registers - (cloneFrom->kernelStack.begin + cloneFrom->kernelStack.length) + (thread->kernelStack.begin + thread->kernelStack.length));
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
    
    REGISTERS_SAVE();

    currentThread->registers = (Registers*)readRegister_RSP_64();

    if (currentThread->process != nextThread->process) {
        PAGING_SWITCH_TO_TABLE(nextThread->process->extendedTable);
    }
    
    context_switch(&currentThread->context, &nextThread->context);

    REGISTERS_RESTORE();

    barrier();

    currentThread->process->lastActiveThread = currentThread;

    if (lastThread->dead) {
        process_notifyThreadDead(lastThread->process, lastThread);
    }
}

void thread_setupForUserProgram(Thread* thread) {
    void* newUserStack = memory_allocateFrame(DIVIDE_ROUND_UP(__THREAD_DEFAULT_USER_STACK_SIZE, PAGE_SIZE));
    if (newUserStack == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    thread->userStack = (Range) {
        .begin = (Uintptr)paging_convertAddressP2V(newUserStack),
        .length = __THREAD_DEFAULT_USER_STACK_SIZE
    };
    memory_memset((void*)thread->userStack.begin, 0, thread->userStack.length);

    ExtendedPageTableRoot* extendedTableRoot = thread->process->extendedTable;
    extendedPageTableRoot_draw(
        extendedTableRoot,
        (void*)thread->userStack.begin,
        newUserStack,
        DIVIDE_ROUND_UP(__THREAD_DEFAULT_USER_STACK_SIZE, PAGE_SIZE),
        extendedTableRoot->context->presets[EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(extendedTableRoot->context, MEMORY_DEFAULT_PRESETS_TYPE_USER_DATA)]
    );

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