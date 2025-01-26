#include<multitask/process.h>

#include<debug.h>
#include<fs/fcntl.h>
#include<fs/fsutil.h>
#include<fs/fsSyscall.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<multitask/schedule.h>
#include<cstring.h>
#include<structs/bitmap.h>
#include<structs/queue.h>
#include<structs/vector.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<real/simpleAsmLines.h>
#include<error.h>

/**
 * @brief Create  process with basic initialization
 * 
 * @param pid PID of process
 * @param name Name of process
 * @return Process* Created process
 */
static Process* __process_create(Uint16 pid, ConstCstring name, void* kernelStackTop);

/**
 * @brief Allocate a PID
 * 
 * @return Uint16 PID allocated
 */
static Uint16 __process_allocatePID();

/**
 * @brief Release a PID
 * 
 * @param pid PID to release
 */
static void __process_releasePID(Uint16 pid);

static Uint16 _process_lastGeneratePID = 0;
static Bitmap _process_pidBitmap;
static Uint8 _process_pidBitmapBits[PROCESS_MAXIMUM_PROCESS_NUM / 8];

__attribute__((aligned(PAGE_SIZE)))
char process_initKernelStack[PROCESS_KERNEL_STACK_SIZE];

static File _process_stdoutFile;

Process* process_init() {
    memory_memset(&_process_pidBitmapBits, 0, sizeof(_process_pidBitmapBits));
    bitmap_initStruct(&_process_pidBitmap, PROCESS_MAXIMUM_PROCESS_NUM, &_process_pidBitmap);
    bitmap_setBit(&_process_pidBitmap, PROCESS_MAIN_PROCESS_RESERVE_PID);

    fs_fileOpen(&_process_stdoutFile, "/dev/stdout", FCNTL_OPEN_WRITE_ONLY);
    ERROR_CHECKPOINT(); //TODO: Temporary solution

    Process* mainProcess = __process_create(PROCESS_MAIN_PROCESS_RESERVE_PID, "Init", process_initKernelStack + PROCESS_KERNEL_STACK_SIZE);
    mainProcess->ppid = PROCESS_MAIN_PROCESS_RESERVE_PID;
    mainProcess->context.extendedTable = mm->extendedTable;

    return mainProcess;
}

void process_switch(Process* from, Process* to) {
    REGISTERS_SAVE();

    from->registers = (Registers*)readRegister_RSP_64();
    
    context_switch(&from->context, &to->context);

    REGISTERS_RESTORE();
}

extern void* __fork_return;

Process* process_fork(ConstCstring name) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    Process* process = scheduler_getCurrentProcess(scheduler);

    Uint32 oldPID = process->pid, newPID = __process_allocatePID();

    if (newPID == PROCESS_INVALID_PID) {
        return NULL;
    }
    
    void* newStack = paging_convertAddressP2V(memory_allocateFrame(DIVIDE_ROUND_UP(PROCESS_KERNEL_STACK_SIZE, PAGE_SIZE)));

    Process* newProcess = __process_create(newPID, name, newStack + PROCESS_KERNEL_STACK_SIZE);
    newProcess->ppid = oldPID;
    ExtendedPageTableRoot* newTable = extendedPageTableRoot_copyTable(process->context.extendedTable);
    Uintptr currentStackTop = process->kernelStackTop;

    REGISTERS_SAVE();
    memory_memcpy(newStack, (void*)(currentStackTop - PROCESS_KERNEL_STACK_SIZE), PROCESS_KERNEL_STACK_SIZE);

    newProcess->context.extendedTable = newTable;
    newProcess->context.rip = (Uint64)&__fork_return;
    newProcess->context.rsp = newProcess->kernelStackTop - (currentStackTop - readRegister_RSP_64());

    scheduler_addProcess(scheduler, newProcess);

    asm volatile("__fork_return:");
    //New process starts from here

    REGISTERS_RESTORE();

    return scheduler_getCurrentProcess(scheduler)->pid == oldPID ? newProcess : NULL;
}

void process_exit() {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    Process* process = scheduler_getCurrentProcess(scheduler);
    scheduler_terminateProcess(scheduler, process);

    debug_blowup("Func process_exit is trying to return\n");
}

void process_release(Process* process) {
    void* stackBottom = (void*)process->kernelStackTop - PROCESS_KERNEL_STACK_SIZE;
    memory_memset(stackBottom, 0, PROCESS_KERNEL_STACK_SIZE);
    memory_freeFrame(paging_convertAddressV2P(stackBottom));

    //TODO: What if user program is running
    extendedPageTableRoot_releaseTable(process->context.extendedTable);

    __process_releasePID(process->pid);

    //TODO: Check these codes again
    for (int i = 1; i < PROCESS_MAX_OPENED_FILE_NUM; ++i) {
        if (process->fileSlots[i] != NULL) {
            fs_fileClose(process->fileSlots[i]);
        }
    }
    Size openedFilePageSize = DIVIDE_ROUND_UP(PROCESS_MAX_OPENED_FILE_NUM * sizeof(File*), PAGE_SIZE);
    memory_memset(process->fileSlots, 0, openedFilePageSize * PAGE_SIZE);
    memory_freeFrame(paging_convertAddressV2P(process->fileSlots));

    memory_memset(process, 0, PAGE_SIZE);
    memory_freeFrame(paging_convertAddressV2P(process));
}

static Process* __process_create(Uint16 pid, ConstCstring name, void* kernelStackTop) {
    Process* ret = paging_convertAddressP2V(memory_allocateFrame(1));
    memory_memset(ret, 0, PAGE_SIZE);

    ret->pid = pid, ret->ppid = 0;
    memory_memcpy(ret->name, name, cstring_strlen(name));

    ret->remainTick = PROCESS_TICK;
    ret->status = PROCESS_STATUS_UNKNOWN;

    ret->kernelStackTop = (Uintptr)kernelStackTop;
    memory_memset(&ret->context, 0, sizeof(Context));
    ret->registers = NULL;
                  
    queueNode_initStruct(&ret->statusQueueNode);
    queueNode_initStruct(&ret->semaWaitQueueNode);

    //TODO: Check these codes again
    Size openedFilePageSize = DIVIDE_ROUND_UP(PROCESS_MAX_OPENED_FILE_NUM * sizeof(File*), PAGE_SIZE);
    ret->fileSlots = paging_convertAddressP2V(frameAllocator_allocateFrame(mm->frameAllocator, openedFilePageSize));
    memory_memset(ret->fileSlots, 0, openedFilePageSize * PAGE_SIZE);

    ret->fileSlots[FSSYSCALL_STANDARD_OUTPUT_FILE_DESCRIPTOR] = &_process_stdoutFile;

    return ret;
}

int process_allocateFileSlot(Process* process, File* file) {
    for (int i = 0; i < PROCESS_MAX_OPENED_FILE_NUM; ++i) {
        if (process->fileSlots[i] == NULL) {
            process->fileSlots[i] = file;
            return i;
        }
    }

    return INVALID_INDEX;
}

File* process_getFileFromSlot(Process* process, int index) {
    if (index >= PROCESS_MAX_OPENED_FILE_NUM) {
        return NULL;
    }

    return process->fileSlots[index];
}

File* process_releaseFileSlot(Process* process, int index) {
    if (index >= PROCESS_MAX_OPENED_FILE_NUM) {
        return NULL;
    }

    File* ret =  process->fileSlots[index];
    process->fileSlots[index] = NULL;
    
    return ret;
}

static Uint16 __process_allocatePID() {
    Uint16 pid = bitmap_findFirstClear(&_process_pidBitmap, _process_lastGeneratePID);
    if (pid != PROCESS_INVALID_PID) {
        bitmap_setBit(&_process_pidBitmap, pid);
        _process_lastGeneratePID = pid;
    }
    return pid;
}

static void __process_releasePID(Uint16 pid) {
    bitmap_clearBit(&_process_pidBitmap, pid);
}