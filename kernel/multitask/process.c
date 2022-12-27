#include<multitask/process.h>

#include<interrupt/IDT.h>
#include<kernel.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/multitaskUtility.h>
#include<memory/paging/directAccess.h>
#include<memory/paging/paging.h>
#include<memory/physicalMemory/pPageAlloc.h>
#include<multitask/schedule.h>
#include<string.h>
#include<structs/bitmap.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>
#include<real/simpleAsmLines.h>

#include<stdio.h>

static Process* _currentProcess = NULL;

static void __handleSwitch(Process* from, Process* to);

static uint16_t __allocatePID();

static void __releasePID(uint16_t pid);

#define CLOBBER_REGISTERS   "rax", "rbx", "rcx", "rdx", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"

uint16_t lastGeneratePID = 0;
Bitmap pidBitmap;
uint8_t pidBitmapBits[MAXIMUM_PROCESS_NUM / 8];

static char* mainProcessName = "Kernel Process";

Process* initProcess() {
    memset(&pidBitmapBits, 0, sizeof(pidBitmapBits));
    initBitmap(&pidBitmap, MAXIMUM_PROCESS_NUM, &pidBitmapBits);
    setBit(&pidBitmap, 0);

    Process* mainProcess = pPageAlloc(1);
    Process* mainProcessDirectAccess = PA_TO_DIRECT_ACCESS_VA(mainProcess);

    memset(mainProcessDirectAccess, 0, sizeof(Process));

    mainProcessDirectAccess->pid = 0;
    mainProcessDirectAccess->pageTable = currentTable;
    initSinglyLinkedListNode(&mainProcessDirectAccess->node);
    memcpy(mainProcessDirectAccess->name, mainProcessName, strlen(mainProcessName));

    //Call switch function, not just for setting up _currentProcess properly, also for setting up the stack pointer properly
    switchProcess(mainProcess, mainProcess);

    changeProcessStatus(mainProcess, PROCESS_STATUS_RUNNING);

    return mainProcess;
}

__attribute__((optimize("O0")))
void switchProcess(Process* from, Process* to) {
    asm volatile(
        "pushfq;"
        "pushq %%rbp;"
        "call %P0;"  //Magic code to call function properly
        "popq %%rbp;"
        "popfq;"
        :
        : "i"(__handleSwitch), "D"(from), "S"(to)
        : "memory", "cc", CLOBBER_REGISTERS
    );
}

#define SWITCH_HANDLE_STACK_PADDING 0x1000

//WARNING: DO NOT MAKE ANY FUNCTION CALL INSIDE THIS FUNCTION, IT MAY CORRUPT THE STACK AND BREAK THE WHOLE SYSTEM(PRINCIPLE STILL UNKNOWN)
//YOU STILL CAN USE SOMETHING LIKE MACRO OR INLINE FUNCTIONS HERE
//Current tests showed that if there is function call inside this function, there might be a chance that stack got corrupted
//The frame of this function is normal, but corruption happens in switchProcess function
//When recovering the registers, the return address might be corrupted, and new process will jump to a unknown position
//What is strange is that, the gdb test showed corruption happens in pop instruction, a shrinking stack affects the memory below
//Or might be something with the timer, still not clear
static void __handleSwitch(Process* from, Process* to) {
    Process* fromDirectAccess = PA_TO_DIRECT_ACCESS_VA(from), * toDirectAccess = PA_TO_DIRECT_ACCESS_VA(to);

    fromDirectAccess->stackTop = (void*)readRegister_RSP_64();

    //Switch the stack, the stack contains the return address
    writeRegister_RSP_64((uint64_t)toDirectAccess->stackTop);
    //Switch the page table
    SWITCH_TO_TABLE(toDirectAccess->pageTable);
    
    _currentProcess = to;
}

Process* getCurrentProcess() {
    return _currentProcess;
}

static uint16_t __allocatePID() {
    size_t pid = findFirstClear(&pidBitmap, lastGeneratePID);
    if (pid != -1) {
        setBit(&pidBitmap, pid);
        lastGeneratePID = pid;
    }
    return pid;
}

static void __releasePID(uint16_t pid) {
    clearBit(&pidBitmap, pid);
}

Process* forkFromCurrentProcess(const char* processName) {
    Process* newProcess = pPageAlloc(1), * oldProcessDirectAccess = PA_TO_DIRECT_ACCESS_VA(_currentProcess);

    uint32_t oldPID = oldProcessDirectAccess->pid, newPID = __allocatePID();
    switchProcess(_currentProcess, _currentProcess);    //Mark the current process's stack here, forked process will take the stack address and starts from here

    uint32_t currentPID = ((Process*)PA_TO_DIRECT_ACCESS_VA(_currentProcess))->pid;
    if (currentPID == oldPID) {   //The parent process is forking new process, forked process will execute this statement too but will not pass
        writeRegister_RSP_64(readRegister_RSP_64() - SWITCH_HANDLE_STACK_PADDING);  //Padding the stack to avoid stack corruption
        
        Process* newProcessDirectAccess = PA_TO_DIRECT_ACCESS_VA(newProcess);

        memset(newProcessDirectAccess, 0, sizeof(Process));

        newProcessDirectAccess->pid = newPID;
        newProcessDirectAccess->stackTop = oldProcessDirectAccess->stackTop;
        
        initSinglyLinkedListNode(&newProcessDirectAccess->node);
        memcpy(newProcessDirectAccess->name, processName, strlen(processName));
        newProcessDirectAccess->pageTable = copyPML4Table(oldProcessDirectAccess->pageTable);

        writeRegister_RSP_64(readRegister_RSP_64() + SWITCH_HANDLE_STACK_PADDING);

        changeProcessStatus(newProcess, PROCESS_STATUS_READY);

        return newProcess;
    }

    //Only forked process will return from here
    return NULL;
}