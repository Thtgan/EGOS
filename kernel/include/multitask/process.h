#if !defined(__PROCESS_H)
#define __PROCESS_H

// #include<fs/file.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<multitask/context.h>
#include<structs/linkedList.h>
#include<structs/queue.h>
#include<structs/vector.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

#define MAXIMUM_PROCESS_NUM         (1u << 16)
#define PROCESS_KERNEL_STACK_SIZE   (4 * PAGE_SIZE)

typedef enum {
    PROCESS_STATUS_UNKNOWN, //Default status for all brand-new created process(After memset 0), shouldn't be used for new status
    PROCESS_STATUS_RUNNING,
    PROCESS_STATUS_READY,
    PROCESS_STATUS_WAITING,
    PROCESS_STATUS_DYING,
    PROCESS_STATUS_NUM      //Num of status, NEVER use this as real process status
} ProcessStatus;

#define PROCESS_TICK                10
#define MAIN_PROCESS_RESERVE_PID    0
#define INVALID_PID                 -1
#define MAX_OPENED_FILE_NUM         (PAGE_SIZE / sizeof(File*))

typedef struct {
    Uint16 pid;
    Uint16 ppid;
    char name[32];
    
    Uint16 remainTick;
    ProcessStatus status;

    Uintptr kernelStackTop;
    Context context;
    Registers* registers;

    // void* userExitStackTop;
    // , * userStackTop, * userProgramBegin;
    // Size userProgramPageSize;
    
    QueueNode statusQueueNode;
    QueueNode semaWaitQueueNode;

    int errorCode;

    // File** fileSlots;
} Process;

/**
 * @brief Set up the processes, get kernel process ready
 * 
 * @return Process* Main process initialized
 */
Process* initProcess();

/**
 * @brief Switch process
 * 
 * @param from Switch from process
 * @param to Switch to process
 */
void switchProcess(Process* from, Process* to);

/**
 * @brief Fork from the current process, a new process will be created, and start from here
 * 
 * @param name New process's name
 * @return Process* Forked process for caller process, return NULL when forked process exit from this function
 */
Process* fork(ConstCstring name);

/**
 * @brief Exit from process, this function has no return
 */
void exitProcess();

/**
 * @brief Release a process
 * 
 * @param process Process to release
 */
void releaseProcess(Process* process);

// /**
//  * @brief Allocate a slot for file
//  * 
//  * @param process Process
//  * @param file File pointer
//  * @return int Index of slot for file, INVALID_INDEX if slots are full
//  */
// int allocateFileSlot(Process* process, File* file);

// /**
//  * @brief Get file from slot
//  * 
//  * @param process Process
//  * @param index Index of slot
//  * @return File* File pointer in slot, NULL if slot is empty
//  */
// File* getFileFromSlot(Process* process, int index);

// /**
//  * @brief Release a file slot
//  * 
//  * @param process Process
//  * @param index Index of slot to release
//  * @return File* File stored in slot, NULL if slot is empty
//  */
// File* releaseFileSlot(Process* process, int index);

#endif // __PROCESS_H
