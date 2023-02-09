#include<multitask/process.h>

#include<debug.h>
#include<interrupt/IDT.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/pageAlloc.h>
#include<memory/paging/paging.h>
#include<memory/paging/pagingCopy.h>
#include<memory/paging/pagingRelease.h>
#include<memory/physicalPages.h>
#include<multitask/schedule.h>
#include<string.h>
#include<structs/bitmap.h>
#include<structs/queue.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<real/simpleAsmLines.h>

static Process* _currentProcess = NULL;

static void __handleSwitch(Process* from, Process* to);

static uint16_t __allocatePID();

static void __releasePID(uint16_t pid);

#define __STACK_PAGE_NUM    ((KERNEL_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE)

#define __CLOBBER_REGISTERS   "rax", "rbx", "rcx", "rdx", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"

static uint16_t _lastGeneratePID = 0;
static Bitmap _pidBitmap;
static uint8_t _pidBitmapBits[MAXIMUM_PROCESS_NUM / 8];

static char* _mainProcessName = "Kernel Process";

Process* initProcess() {
    memset(&_pidBitmapBits, 0, sizeof(_pidBitmapBits));
    initBitmap(&_pidBitmap, MAXIMUM_PROCESS_NUM, &_pidBitmap);
    setBit(&_pidBitmap, MAIN_PROCESS_RESERVE_PID);

    Process* mainProcess = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);

    memset(mainProcess, 0, sizeof(Process));

    mainProcess->pid = mainProcess->ppid = MAIN_PROCESS_RESERVE_PID;
    mainProcess->remainTick = PROCESS_TICK;
    mainProcess->pageTable = currentPageTable;
    initQueueNode(&mainProcess->statusQueueNode);
    initQueueNode(&mainProcess->semaWaitQueueNode);
    memcpy(mainProcess->name, _mainProcessName, strlen(_mainProcessName));

    //Call switch function, not just for setting up _currentProcess properly, also for setting up the stack pointer properly
    switchProcess(mainProcess, mainProcess);

    PhysicalPage* stackPhysicalPageStruct = getPhysicalPageStruct(translateVaddr(currentPageTable, (void*)(KERNEL_PHYSICAL_END | KERNEL_VIRTUAL_BEGIN)));
    for (int i = 0; i < __STACK_PAGE_NUM; ++i) {
        (stackPhysicalPageStruct + i)->flags = PHYSICAL_PAGE_TYPE_NORMAL;
        referPhysicalPage(stackPhysicalPageStruct + i);
    }

    setProcessStatus(mainProcess, PROCESS_STATUS_RUNNING);

    return mainProcess;
}

__attribute__((optimize("O0")))
void switchProcess(Process* from, Process* to) {
    asm volatile(
        "pushfq;"
        "pushq %%rbp;"
        // "movq %%rsp, %P3(%1);"   //TODO: It is weird that swapping stacks here causes unknown error, must be corrupted somewhere, nail it in future
        // "movq %P3(%2), %%rsp;"
        "call %P0;"
        "popq %%rbp;"
        "popfq;"
        :
        : "i"(__handleSwitch), "D"(from), "S"(to)//, "i" (offsetof(Process, stackTop))
        : "memory", "cc", __CLOBBER_REGISTERS
    );
}

//WARNING: DO NOT MAKE ANY FUNCTION CALL BETWEEN STACK SWAP AND PAGE TABLE SWITCH, IT WILL CAUSE UNKNOWN RESULTS
static void __handleSwitch(Process* from, Process* to) {
    from->stackTop = (void*)readRegister_RSP_64();
    //Switch the stack, the stack contains the return address
    writeRegister_RSP_64((uint64_t)to->stackTop);

    //Switch the page table
    SWITCH_TO_TABLE(to->pageTable);
    
    _currentProcess = to;
}

Process* getCurrentProcess() {
    return _currentProcess;
}

static char _tmpStack[KERNEL_STACK_SIZE];

Process* forkFromCurrentProcess(const char* processName) {
    uint32_t oldPID = _currentProcess->pid, newPID = __allocatePID();
    if (newPID == INVALID_PID) {
        return NULL;
    }

    Process* newProcess = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);

    switchProcess(_currentProcess, _currentProcess);    //Mark the current process's stack here, forked process will take the stack address and starts from here
    char* source = (char*)KERNEL_STACK_BOTTOM - KERNEL_STACK_SIZE;
    for (int i = 0; i < KERNEL_STACK_SIZE; ++i) {
        _tmpStack[i] = source[i];
    }

    uint32_t currentPID = _currentProcess->pid;
    if (currentPID == oldPID) {   //The parent process is forking new process, forked process will execute this statement too but will not pass
        memset(newProcess, 0, sizeof(Process));

        newProcess->pid = newPID;
        newProcess->ppid = oldPID;
        newProcess->remainTick = PROCESS_TICK;
        memcpy(newProcess->name, processName, strlen(processName));
        newProcess->pageTable = copyPML4Table(_currentProcess->pageTable);
        newProcess->stackTop = _currentProcess->stackTop;

        void* newStackBottom = pageAlloc(__STACK_PAGE_NUM, PHYSICAL_PAGE_TYPE_NORMAL) + KERNEL_STACK_SIZE;
        for (uintptr_t i = PAGE_SIZE; i <= KERNEL_STACK_SIZE; i += PAGE_SIZE) {
            mapAddr(newProcess->pageTable, (void*)KERNEL_STACK_BOTTOM - i, newStackBottom - i);
        }

        char* des = newStackBottom - KERNEL_STACK_SIZE;
        for (int i = 0; i < KERNEL_STACK_SIZE; ++i) {
            des[i] = _tmpStack[i];
        }

        initQueueNode(&newProcess->statusQueueNode);
        initQueueNode(&newProcess->semaWaitQueueNode);

        setProcessStatus(newProcess, PROCESS_STATUS_READY);

        return newProcess;
    }

    //Only forked process will return from here
    return NULL;
}

void exitProcess() {
    schedule(PROCESS_STATUS_DYING);

    blowup("Func exitProcess is trying to return\n");
}

void releaseProcess(Process* process) {
    void* pAddr = translateVaddr(process->pageTable, (void*)(KERNEL_STACK_BOTTOM - KERNEL_STACK_SIZE));
    memset(pAddr, 0, KERNEL_STACK_SIZE);
    pageFree(pAddr, __STACK_PAGE_NUM);

    releasePML4Table(process->pageTable);

    __releasePID(process->pid);
    memset(process, 0, PAGE_SIZE);
    pageFree(process, 1);
}

static uint16_t __allocatePID() {
    uint16_t pid = findFirstClear(&_pidBitmap, _lastGeneratePID);
    if (pid != INVALID_PID) {
        setBit(&_pidBitmap, pid);
        _lastGeneratePID = pid;
    }
    return pid;
}

static void __releasePID(uint16_t pid) {
    clearBit(&_pidBitmap, pid);
}
