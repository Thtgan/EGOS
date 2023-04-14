#if !defined(__PROCESS_H)
#define __PROCESS_H

#include<error.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<structs/linkedList.h>
#include<structs/queue.h>
#include<structs/registerSet.h>
#include<structs/vector.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

#define MAXIMUM_PROCESS_NUM (1u << 16)

typedef struct _Process Process;

typedef enum {
    PROCESS_STATUS_UNKNOWN, //Default status for all brand-new created process(After memset 0), shouldn't be used for new status
    PROCESS_STATUS_RUNNING,
    PROCESS_STATUS_READY,
    PROCESS_STATUS_WAITING,
    PROCESS_STATUS_DYING,
    PROCESS_STATUS_NUM      //Num of status, NEVER use this as real process status
} ProcessStatus;

#define PROCESS_TICK                5
#define MAIN_PROCESS_RESERVE_PID    0
#define INVALID_PID                 -1

struct _Process {
    uint16_t pid;
    uint16_t ppid;
    char name[32];
    
    uint16_t remainTick;
    ProcessStatus status;

    PML4Table* pageTable;
    RegisterSet* registers;

    void* userExitStackTop, * userStackTop, * userProgramBegin;
    size_t userProgramPageSize;
    
    QueueNode statusQueueNode;
    QueueNode semaWaitQueueNode;

    int errorCode;
};

/**
 * @brief Set up the processes, get kernel process ready
 * 
 * @return Process* Main process initialized
 */
Process* initProcess();

/**
 * @brief Initialize TSS
 */
void initTSS();

/**
 * @brief Switch process
 * 
 * @param from Switch from process
 * @param to Switch to process
 */
void switchProcess(Process* from, Process* to);

/**
 * @brief Get current process
 * 
 * @return Process* Current process
 */
Process* getCurrentProcess();

/**
 * @brief Fork from the current process, a new process will be created, and start from here
 * 
 * @param processName New process's name
 * @return Process* Forked process for caller process, return NULL when forked process exit from this function
 */
Process* forkFromCurrentProcess(const char* processName);

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

#endif // __PROCESS_H
