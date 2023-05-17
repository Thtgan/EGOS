#include<multitask/process.h>

#include<debug.h>
#include<fs/file.h>
#include<fs/fsutil.h>
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
#include<structs/registerSet.h>
#include<structs/vector.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<system/GDT.h>
#include<system/TSS.h>
#include<real/simpleAsmLines.h>

static TSS _tss;

static Process* _currentProcess = NULL;

/**
 * @brief Create  process with basic initialization
 * 
 * @param pid PID of process
 * @param name Name of process
 * @return Process* Created process
 */
static Process* __createProcess(uint16_t pid, ConstCstring name);

/**
 * @brief Allocate a PID
 * 
 * @return uint16_t PID allocated
 */
static uint16_t __allocatePID();

/**
 * @brief Release a PID
 * 
 * @param pid PID to release
 */
static void __releasePID(uint16_t pid);

static uint16_t _lastGeneratePID = 0;
static Bitmap _pidBitmap;
static uint8_t _pidBitmapBits[MAXIMUM_PROCESS_NUM / 8];

Process* initProcess() {
    memset(&_pidBitmapBits, 0, sizeof(_pidBitmapBits));
    initBitmap(&_pidBitmap, MAXIMUM_PROCESS_NUM, &_pidBitmap);
    setBit(&_pidBitmap, MAIN_PROCESS_RESERVE_PID);

    Process* mainProcess = __createProcess(MAIN_PROCESS_RESERVE_PID, "Kernel Process");
    mainProcess->ppid = MAIN_PROCESS_RESERVE_PID;
    mainProcess->pageTable = currentPageTable;

    switchProcess(mainProcess, mainProcess);

    //Call switch function, not just for setting up _currentProcess properly, also for setting up the stack pointer properly
    PhysicalPage* stackPhysicalPageStruct = getPhysicalPageStruct(translateVaddr(currentPageTable, (void*)(KERNEL_PHYSICAL_END | KERNEL_VIRTUAL_BEGIN)));
    for (int i = 0; i < KERNEL_STACK_PAGE_NUM; ++i) {
        (stackPhysicalPageStruct + i)->flags = PHYSICAL_PAGE_TYPE_NORMAL;
        referPhysicalPage(stackPhysicalPageStruct + i);
    }

    setProcessStatus(mainProcess, PROCESS_STATUS_RUNNING);

    return mainProcess;
}

void initTSS() {
    memset(&_tss, 0, sizeof(TSS));
    _tss.ist[0] = (uintptr_t)pageAlloc(2, PHYSICAL_PAGE_TYPE_PUBLIC);
    _tss.rsp[0] = (uintptr_t)pageAlloc(2, PHYSICAL_PAGE_TYPE_PUBLIC);
    _tss.ioMapBaseAddress = 0x8000;  //Invalid
    
    GDTDesc64* desc = (GDTDesc64*)sysInfo->gdtDesc;
    GDTEntryTSS_LDT* gdtEntryTSS = (GDTEntryTSS_LDT*)((GDTEntry*)desc->table + GDT_ENTRY_INDEX_TSS);

    *gdtEntryTSS = BUILD_GDT_ENTRY_TSS_LDT(((uintptr_t)&_tss), sizeof(TSS), GDT_TSS_LDT_TSS | GDT_TSS_LDT_PRIVIEGE_0 | GDT_TSS_LDT_PRESENT, 0);

    asm volatile("ltr %w0" :: "r"(SEGMENT_TSS));
}

__attribute__((naked))
void switchProcess(Process* from, Process* to) {
    asm volatile(
        SAVE_ALL
    );
    asm volatile(
        "mov %%rsp, %0;"
        "mov %1, %%rsp"
        : "=m"(from->registers)
        : "m"(to->registers)
    );

    //Switch the page table
    SWITCH_TO_TABLE(to->pageTable);
    
    _currentProcess = to;

    asm volatile(
        RESTORE_ALL
        "retq;"
    );
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

    switchProcess(_currentProcess, _currentProcess);    //Mark the current process's stack here, forked process will take the stack address and starts from here
    char* source = (char*)KERNEL_STACK_BOTTOM - KERNEL_STACK_SIZE;
    for (int i = 0; i < KERNEL_STACK_SIZE; ++i) {
        _tmpStack[i] = source[i];
    }

    uint32_t currentPID = _currentProcess->pid;
    if (currentPID == oldPID) {   //The parent process is forking new process, forked process will execute this statement too but will not pass
        Process* newProcess = __createProcess(newPID, processName);
        newProcess->ppid = oldPID;
        newProcess->pageTable = copyPML4Table(_currentProcess->pageTable);
        newProcess->registers = _currentProcess->registers;

        void* newStackBottom = pageAlloc(KERNEL_STACK_PAGE_NUM, PHYSICAL_PAGE_TYPE_NORMAL) + KERNEL_STACK_SIZE;
        for (uintptr_t i = PAGE_SIZE; i <= KERNEL_STACK_SIZE; i += PAGE_SIZE) {
            mapAddr(newProcess->pageTable, (void*)KERNEL_STACK_BOTTOM - i, newStackBottom - i);
        }

        char* des = newStackBottom - KERNEL_STACK_SIZE;
        for (int i = 0; i < KERNEL_STACK_SIZE; ++i) {
            des[i] = _tmpStack[i];
        }

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
    pageFree(pAddr, KERNEL_STACK_PAGE_NUM);

    if (process->userStackTop != NULL) {
        for (uintptr_t i = PAGE_SIZE; i <= USER_STACK_SIZE; i += PAGE_SIZE) {
            pageFree(translateVaddr(process->pageTable, (void*)USER_STACK_BOTTOM - i), 1);
        }
    }

    if (process->userProgramBegin != NULL) {
        pageFree(process->userProgramBegin, process->userProgramPageSize);
    }

    releasePML4Table(process->pageTable);

    __releasePID(process->pid);

    for (int i = 0; i < MAX_OPENED_FILE_NUM; ++i) {
        if (process->fileSlots[i] != NULL) {
            fileClose(process->fileSlots[i]);
        }
    }
    size_t openedFilePageSize = (MAX_OPENED_FILE_NUM * sizeof(File*) + PAGE_SIZE - 1) / PAGE_SIZE;
    memset(process->fileSlots, 0, openedFilePageSize * PAGE_SIZE);
    pageFree(process->fileSlots, (MAX_OPENED_FILE_NUM * sizeof(File*) + PAGE_SIZE - 1) / PAGE_SIZE);


    memset(process, 0, PAGE_SIZE);
    pageFree(process, 1);
}

static Process* __createProcess(uint16_t pid, ConstCstring name) {
    Process* ret = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);
    memset(ret, 0, PAGE_SIZE);

    ret->pid = pid, ret->ppid = 0;
    memcpy(ret->name, name, strlen(name));

    ret->remainTick = PROCESS_TICK;
    ret->status = PROCESS_STATUS_UNKNOWN;

    ret->pageTable = NULL;
    ret->registers = NULL;

    ret->userStackTop = ret->userProgramBegin = NULL;
    ret->userProgramPageSize = 0;

    initQueueNode(&ret->statusQueueNode);
    initQueueNode(&ret->semaWaitQueueNode);

    size_t openedFilePageSize = (MAX_OPENED_FILE_NUM * sizeof(File*) + PAGE_SIZE - 1) / PAGE_SIZE;
    ret->fileSlots = pageAlloc(openedFilePageSize, PHYSICAL_PAGE_TYPE_PRIVATE);
    memset(ret->fileSlots, 0, openedFilePageSize * PAGE_SIZE);

    File* ttyFile = NULL;
    if ((ttyFile = fileOpen("/dev/tty", FILE_FLAG_READ_WRITE)) != NULL) {
        ret->fileSlots[0] = ttyFile;
    }

    return ret;
}

int allocateFileSlot(Process* process, File* file) {
    for (int i = 0; i < MAX_OPENED_FILE_NUM; ++i) {
        if (process->fileSlots[i] == NULL) {
            process->fileSlots[i] = file;
            return i;
        }
    }

    return INVALID_INDEX;
}

File* getFileFromSlot(Process* process, int index) {
    if (index >= MAX_OPENED_FILE_NUM) {
        return NULL;
    }

    return process->fileSlots[index];
}

File* releaseFileSlot(Process* process, int index) {
    if (index >= MAX_OPENED_FILE_NUM) {
        return NULL;
    }

    File* ret =  process->fileSlots[index];
    process->fileSlots[index] = NULL;
    
    return ret;
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
