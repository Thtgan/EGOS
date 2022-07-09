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

Process* initProcess();

void switchProcess(Process* from, Process* to);

Process* getCurrentProcess();

Process* forkFromCurrentProcess(const char* processName);

#endif // __PROCESS_H
