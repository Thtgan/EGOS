#include<multitask/process.h>

#include<debug.h>
#include<devices/virtualDevice.h>
// #include<fs/file.h>
// #include<fs/fsutil.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/paging/pagingCopy.h>
#include<memory/paging/pagingRelease.h>
#include<memory/physicalPages.h>
#include<multitask/context.h>
#include<multitask/schedule.h>
#include<string.h>
#include<structs/bitmap.h>
#include<structs/queue.h>
#include<structs/vector.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<real/simpleAsmLines.h>

/**
 * @brief Create  process with basic initialization
 * 
 * @param pid PID of process
 * @param name Name of process
 * @return Process* Created process
 */
static Process* __createProcess(Uint16 pid, ConstCstring name, void* kernelStackTop);

/**
 * @brief Allocate a PID
 * 
 * @return Uint16 PID allocated
 */
static Uint16 __allocatePID();

/**
 * @brief Release a PID
 * 
 * @param pid PID to release
 */
static void __releasePID(Uint16 pid);

static Uint16 _lastGeneratePID = 0;
static Bitmap _pidBitmap;
static Uint8 _pidBitmapBits[MAXIMUM_PROCESS_NUM / 8];

__attribute__((aligned(PAGE_SIZE)))
char initKernelStack[PROCESS_KERNEL_STACK_SIZE];

Process* initProcess() {
    memset(&_pidBitmapBits, 0, sizeof(_pidBitmapBits));
    bitmap_initStruct(&_pidBitmap, MAXIMUM_PROCESS_NUM, &_pidBitmap);
    bitmap_setBit(&_pidBitmap, MAIN_PROCESS_RESERVE_PID);

    Process* mainProcess = __createProcess(MAIN_PROCESS_RESERVE_PID, "Init", initKernelStack + PROCESS_KERNEL_STACK_SIZE);
    mainProcess->ppid = MAIN_PROCESS_RESERVE_PID;
    mainProcess->context.pageTable = mm->currentPageTable;

    return mainProcess;
}

void switchProcess(Process* from, Process* to) {
    SAVE_REGISTERS();

    from->registers = (Registers*)readRegister_RSP_64();
    
    switchContext(&from->context, &to->context);

    RESTORE_REGISTERS();
}

extern void* __fork_return;

Process* fork(ConstCstring name) {
    Uint32 oldPID = schedulerGetCurrentProcess()->pid, newPID = __allocatePID();

    if (newPID == INVALID_PID) {
        return NULL;
    }
    
    void* newStack = convertAddressP2V(pageAlloc(DIVIDE_ROUND_UP(PROCESS_KERNEL_STACK_SIZE, PAGE_SIZE), MEMORY_TYPE_NORMAL));

    Process* newProcess = __createProcess(newPID, name, newStack + 4 * PAGE_SIZE);
    newProcess->ppid = oldPID;
    PML4Table* newTable = copyPML4Table(schedulerGetCurrentProcess()->context.pageTable);

    // memset(newStack, 0, KERNEL_STACK_SIZE);
    // for (Uintptr i = PAGE_SIZE; i <= KERNEL_STACK_SIZE; i += PAGE_SIZE) {
    //     mapAddr(newTable, (void*)KERNEL_STACK_BOTTOM - i, newStack + KERNEL_STACK_SIZE - i);
    // }

    SAVE_REGISTERS();
    memcpy(newStack, (void*)schedulerGetCurrentProcess()->kernelStackTop - PROCESS_KERNEL_STACK_SIZE, PROCESS_KERNEL_STACK_SIZE);

    newProcess->context.pageTable = newTable;
    newProcess->context.rip = (Uint64)&__fork_return;
    newProcess->context.rsp = newProcess->kernelStackTop - (schedulerGetCurrentProcess()->kernelStackTop - readRegister_RSP_64());

    schedulerAddProcess(newProcess);

    asm volatile("__fork_return:");
    //New process starts from here

    RESTORE_REGISTERS();

    return schedulerGetCurrentProcess()->pid == oldPID ? newProcess : NULL;
}

void exitProcess() {
    schedulerTerminateProcess(schedulerGetCurrentProcess());

    debug_belowup("Func exitProcess is trying to return\n");
}

void releaseProcess(Process* process) {
    PML4Table* pageTable = process->context.pageTable;
    void* stackBottom = (void*)process->kernelStackTop - PROCESS_KERNEL_STACK_SIZE;
    memset(stackBottom, 0, PROCESS_KERNEL_STACK_SIZE);
    pageFree(convertAddressV2P(stackBottom));

    // if (process->userStackTop != NULL) {
    //     for (Uintptr i = PAGE_SIZE; i <= USER_STACK_SIZE; i += PAGE_SIZE) {
    //         pageFree(translateVaddr(pageTable, (void*)USER_STACK_BOTTOM - i));
    //     }
    // }

    // if (process->userProgramBegin != NULL) {
    //     pageFree(process->userProgramBegin);
    // }

    releasePML4Table(pageTable);

    __releasePID(process->pid);

    // for (int i = 1; i < MAX_OPENED_FILE_NUM; ++i) {
    //     if (process->fileSlots[i] != NULL) {
    //         fileClose(process->fileSlots[i]);
    //     }
    // }
    // Size openedFilePageSize = (MAX_OPENED_FILE_NUM * sizeof(File*) + PAGE_SIZE - 1) / PAGE_SIZE;
    // memset(process->fileSlots, 0, openedFilePageSize * PAGE_SIZE);
    // pageFree(process->fileSlots);


    memset(process, 0, PAGE_SIZE);
    pageFree(convertAddressV2P(process));
}

static Process* __createProcess(Uint16 pid, ConstCstring name, void* kernelStackTop) {
    Process* ret = convertAddressP2V(pageAlloc(1, MEMORY_TYPE_PRIVATE));
    memset(ret, 0, PAGE_SIZE);

    ret->pid = pid, ret->ppid = 0;
    memcpy(ret->name, name, strlen(name));

    ret->remainTick = PROCESS_TICK;
    ret->status = PROCESS_STATUS_UNKNOWN;

    ret->kernelStackTop = (Uintptr)kernelStackTop;
    memset(&ret->context, 0, sizeof(Context));
    ret->registers = NULL;

    // ret->userStackTop = ret->userProgramBegin = NULL;
    // ret->userProgramPageSize = 0;

    queueNode_initStruct(&ret->statusQueueNode);
    queueNode_initStruct(&ret->semaWaitQueueNode);

    // Size openedFilePageSize = (MAX_OPENED_FILE_NUM * sizeof(File*) + PAGE_SIZE - 1) / PAGE_SIZE;
    // ret->fileSlots = pageAlloc(openedFilePageSize, MEMORY_TYPE_PRIVATE);
    // memset(ret->fileSlots, 0, openedFilePageSize * PAGE_SIZE);

    // ret->fileSlots[0] = getStandardOutputFile();

    return ret;
}

// int allocateFileSlot(Process* process, File* file) {
//     for (int i = 0; i < MAX_OPENED_FILE_NUM; ++i) {
//         if (process->fileSlots[i] == NULL) {
//             process->fileSlots[i] = file;
//             return i;
//         }
//     }

//     return INVALID_INDEX;
// }

// File* getFileFromSlot(Process* process, int index) {
//     if (index >= MAX_OPENED_FILE_NUM) {
//         return NULL;
//     }

//     return process->fileSlots[index];
// }

// File* releaseFileSlot(Process* process, int index) {
//     if (index >= MAX_OPENED_FILE_NUM) {
//         return NULL;
//     }

//     File* ret =  process->fileSlots[index];
//     process->fileSlots[index] = NULL;
    
//     return ret;
// }

static Uint16 __allocatePID() {
    Uint16 pid = bitmap_findFirstClear(&_pidBitmap, _lastGeneratePID);
    if (pid != INVALID_PID) {
        bitmap_setBit(&_pidBitmap, pid);
        _lastGeneratePID = pid;
    }
    return pid;
}

static void __releasePID(Uint16 pid) {
    bitmap_clearBit(&_pidBitmap, pid);
}