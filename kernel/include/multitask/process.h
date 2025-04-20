#if !defined(__MULTITASK_PROCESS_H)
#define __MULTITASK_PROCESS_H

typedef struct Process Process;

#include<fs/fsEntry.h>
#include<kit/types.h>
#include<memory/extendedPageTable.h>
#include<multitask/state.h>
#include<multitask/thread.h>
#include<structs/linkedList.h>
#include<structs/string.h>
#include<structs/vector.h>
#include<system/pageTable.h>

typedef struct Process {
    Uint16 pid;
    Uint16 ppid;
    String name;
    
    State state;

    ExtendedPageTableRoot* extendedTable;

    Thread* lastActiveThread;
    LinkedList threads;

    Vector fsEntries;

    LinkedList childProcesses;
    LinkedListNode childProcessNode;

    LinkedListNode scheduleNode;

    bool isProcessActive;
} Process;

void process_initStruct(Process* process, Uint16 pid, ConstCstring name, ExtendedPageTableRoot* extendedTable);

void process_clone(Process* process, Uint16 pid, Process* cloneFrom);

void process_clearStruct(Process* process);

void proesss_stop(Process* process, int signal);    //TODO: signal not used yet

void process_kill(Process* process, int signal);    //TODO: signal not used yet

void process_addThread(Process* process, Thread* thread);

Thread* process_getThreadFromTID(Process* process, Uint16 tid);

void process_removeThread(Process* process, Thread* thread);

Thread* process_createThread(Process* process, ThreadEntryPoint entry);

void process_notifyThreadDead(Process* process, Thread* thread);

void process_setParent(Process* process, Process* parent);

fsEntry* process_getFSentry(Process* process, int fd);

int process_addFSentry(Process* process, fsEntry* entry);

fsEntry* process_removeFSentry(Process* process, int fd);

#endif // __MULTITASK_PROCESS_H
