#include<multitask/process.h>

#include<interrupt/IDT.h>
#include<memory/memory.h>
#include<memory/multitaskUtility.h>
#include<memory/paging/tableSwitch.h>
#include<memory/virtualMalloc.h>
#include<stddef.h>
#include<stdint.h>
#include<string.h>
#include<structs/bitmap.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>
#include<real/simpleAsmLines.h>

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

    Process* mainProcess = vMalloc(sizeof(Process));

    mainProcess->pid = 0;
    mainProcess->status = PROCESS_STATUS_RUNNING;
    mainProcess->pageTable = getCurrentTable();
    initSinglyLinkedListNode(&mainProcess->node);
    memcpy(mainProcess->name, mainProcessName, strlen(mainProcessName));

    //Call switch function, not just for setting up _currentProcess properly, also for setting up the stack pointer properly
    switchProcess(mainProcess, mainProcess);

    return mainProcess;
}

__attribute__((optimize("O0")))
void switchProcess(Process* from, Process* to) {
    asm volatile(
        "call %P0"  //Magic code to call function properly
        :
        : "i"(__handleSwitch), "D"(from), "S"(to)
        : "memory", "cc", CLOBBER_REGISTERS
    );
}

#define SWITCH_HANDLE_STACK_PADDING 0x1000

static void __handleSwitch(Process* from, Process* to) {
    bool enabled = disableInterrupt();

    pushfq();
    pushRegister_RBP_64();

    from->stackTop = (void*)readRegister_RSP_64();
    from->status = PROCESS_STATUS_READY;

    //Switch the page table
    markCurrentTable(to->pageTable);
    writeRegister_CR3_64((uint64_t)to->pageTable);

    //No function calls between codes here (May corrupt the stack)

    //Switch the stack, the stack contains the return address
    writeRegister_RSP_64((uint64_t)to->stackTop);
    
    to->status = PROCESS_STATUS_RUNNING;
    _currentProcess = to;

    popRegister_RBP_64();
    popfq();

    setInterrupt(enabled);
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
    Process* newProcess = vMalloc(sizeof(Process));

    uint32_t oldPID = _currentProcess->pid, newPID = __allocatePID();
    switchProcess(_currentProcess, _currentProcess);    //Mark the current process's stack here, forked process will take the stack address and starts from here

    if (_currentProcess->pid == oldPID) {   //The parent process is forking new process, forked process will execute this statement too but will not pass
        newProcess->pid = newPID;
        newProcess->status = PROCESS_STATUS_READY;
        newProcess->stackTop = _currentProcess->stackTop;
        
        writeRegister_RSP_64(readRegister_RSP_64() - SWITCH_HANDLE_STACK_PADDING);  //Padding the stack to avoid stack corruption
        initSinglyLinkedListNode(&newProcess->node);
        memcpy(newProcess->name, processName, strlen(processName));
        newProcess->pageTable = copyPML4Table(_currentProcess->pageTable);
        writeRegister_RSP_64(readRegister_RSP_64() + SWITCH_HANDLE_STACK_PADDING);

        return newProcess;
    }

    //Only forked process will return from here
    return NULL;
}