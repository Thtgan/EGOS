#if !defined(__PROCESS_H)
#define __PROCESS_H

#include<stdint.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

#define MAXIMUM_PROCESS_NUM (1u << 16)

typedef struct _Process Process;

typedef enum {
    PROCESS_STATUS_RUNNING,
    PROCESS_STATUS_READY,
    PROCESS_STATUS_WAITING,
    PROCESS_STATUS_NEW,
    PROCESS_STATUS_DEAD
} ProcessStatus;

struct _Process {
    uint16_t pid;
    ProcessStatus status;
    PML4Table* pageTable;
    void* stackTop;
    SinglyLinkedListNode node;
    char name[32];
};

/**
 * @brief Set up the processes, get kernel process ready
 * 
 * @return Process* 
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

#endif // __PROCESS_H
