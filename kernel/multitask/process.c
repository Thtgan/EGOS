#include<multitask/process.h>

#include<fs/fcntl.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<multitask/schedule.h>
#include<multitask/state.h>
#include<multitask/thread.h>
#include<structs/linkedList.h>
#include<structs/string.h>
#include<structs/vector.h>
#include<system/pageTable.h>
#include<error.h>

void process_initStruct(Process* process, Uint16 pid, ConstCstring name, ExtendedPageTableRoot* extendedTable) {
    process->pid = pid;
    process->ppid = 0;

    string_initStructStr(&process->name, name);
    ERROR_GOTO_IF_ERROR(0);

    process->state = STATE_RUNNING;

    process->extendedTable = extendedTable;

    process->lastActiveThread = NULL;
    linkedList_initStruct(&process->threads);

    vector_initStruct(&process->fsEntries);
    ERROR_GOTO_IF_ERROR(0);

    File* stdout = fs_fileOpen("/dev/stdout", FCNTL_OPEN_WRITE_ONLY);
    ERROR_GOTO_IF_ERROR(1);
    
    vector_push(&process->fsEntries, (Object)stdout);
    ERROR_GOTO_IF_ERROR(2);

    linkedList_initStruct(&process->childProcesses);
    linkedListNode_initStruct(&process->childProcessNode);

    linkedListNode_initStruct(&process->scheduleNode);
    
    process->isProcessActive = false;

    return;
    ERROR_FINAL_BEGIN(2);
    fs_fileClose(stdout);

    ERROR_FINAL_BEGIN(1);
    vector_clearStruct(&process->fsEntries);

    ERROR_FINAL_BEGIN(0);

    if (string_isAvailable(&process->name)) {
        string_clearStruct(&process->name);
    }
}

void process_clone(Process* process, Uint16 pid, Process* cloneFrom) {
    process_initStruct(process, pid, cloneFrom->name.data, NULL);   //TODO: Temporary NULL, need a new page table copy routine
    ERROR_GOTO_IF_ERROR(0);

    vector_resize(&process->fsEntries, cloneFrom->fsEntries.capacity);
    Size fsEntryNum = process->fsEntries.size;
    for (int i = 0; i < fsEntryNum; ++i) {
        Object entry = vector_get(&process->fsEntries, i);
        ERROR_GOTO_IF_ERROR(0);

        if (entry == OBJECT_NULL) {
            vector_push(&process->fsEntries, OBJECT_NULL);
        } else {
            fsEntry* newEntry = fsEntry_copy((fsEntry*)entry);
            DEBUG_ASSERT_SILENT(newEntry != NULL);
            vector_push(&process->fsEntries, (Object)newEntry);
        }

        ERROR_GOTO_IF_ERROR(0);
    }

    if (cloneFrom->lastActiveThread != NULL) {
        DEBUG_ASSERT_SILENT(cloneFrom->lastActiveThread->state == STATE_RUNNING);
    
        Thread* newThread = memory_allocate(sizeof(Thread));
        if (newThread == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
    
        Uint16 tid = schedule_allocateNewID();
        thread_clone(newThread, cloneFrom->lastActiveThread, tid, process);
        ERROR_GOTO_IF_ERROR(0);

        if (schedule_getCurrentThread()->tid != tid) {
            process_addThread(process, newThread);
    
            process->lastActiveThread = newThread;
        }
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void process_clearStruct(Process* process) {
    DEBUG_ASSERT_SILENT(process != schedule_getCurrentProcess());
    DEBUG_ASSERT_SILENT(linkedList_isEmpty(&process->threads));

    Size fsEntryNum = process->fsEntries.size;
    for (int i = 0; i < fsEntryNum; ++i) {
        fsEntry* entry = (File*)vector_get(&process->fsEntries, i);
        ERROR_GOTO_IF_ERROR(0);

        if (entry == NULL) {
            continue;
        }
        
        fs_fileClose(entry);
        ERROR_GOTO_IF_ERROR(0);
    }

    vector_clearStruct(&process->fsEntries);

    string_clearStruct(&process->name);

    return;
    ERROR_FINAL_BEGIN(0);
}

void proesss_stop(Process* process, int signal) {   //TODO: signal not used yet
    Thread* currentThread = schedule_getCurrentThread();
    thread_refer(currentThread);    //To prevent yielding when stopping current thread
    for (LinkedListNode* node = linkedListNode_getNext(&process->threads); node != &process->threads; node = node->next) {
        Thread* thread = HOST_POINTER(node, Thread, processNode);
        
        thread_stop(thread);
    }
    thread_derefer(currentThread);
}

void process_kill(Process* process, int signal) {   //TODO: signal not used yet
    Thread* currentThread = schedule_getCurrentThread();
    thread_refer(currentThread);    //To prevent yielding when stopping current thread
    for (LinkedListNode* node = linkedListNode_getNext(&process->threads); node != &process->threads; node = node->next) {
        Thread* thread = HOST_POINTER(node, Thread, processNode);
        
        thread_die(thread);
    }

    thread_derefer(currentThread);
}

void process_addThread(Process* process, Thread* thread) {
    if (process->lastActiveThread == NULL) {
        process->lastActiveThread = thread;
    }
    linkedListNode_insertFront(&process->threads, &thread->processNode);

    if (process->isProcessActive) {
        schedule_addThread(thread);
    }
}

Thread* process_getThreadFromTID(Process* process, Uint16 tid) {
    for (LinkedListNode* node = linkedListNode_getNext(&process->threads); node != &process->threads; node = node->next) {
        Thread* thread = HOST_POINTER(node, Thread, processNode);
        if (thread->tid == tid) {
            return thread;
        }
    }

    return NULL;
}

void process_removeThread(Process* process, Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->process == process);
    DEBUG_ASSERT_SILENT(thread != schedule_getCurrentThread());
    linkedListNode_delete(&thread->processNode);
    if (process->lastActiveThread == thread) {
        process->lastActiveThread = NULL;
    }

    if (process->isProcessActive) {
        schedule_removeThread(thread);
    }
}

Thread* process_createThread(Process* process, ThreadEntryPoint entry) {
    Thread* newThread = memory_allocate(sizeof(Thread));
    if (newThread == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Uint16 tid = schedule_allocateNewID();
    thread_initStruct(newThread, tid, process, entry, NULL);
    ERROR_GOTO_IF_ERROR(0);

    process_addThread(process, newThread);

    return newThread;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void process_notifyThreadDead(Process* process, Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->process == process);
    DEBUG_ASSERT_SILENT(thread != schedule_getCurrentThread());
    DEBUG_ASSERT_SILENT(!linkedList_isEmpty(&process->threads));
    
    process_removeThread(process, thread);
    if (!linkedList_isEmpty(&process->threads)) {
        return;
    }
    
    DEBUG_ASSERT_SILENT(process != schedule_getCurrentProcess());
    if (!linkedList_isEmpty(&process->childProcesses)) {
        schedule_collectOrphans(process);
    }
}

void process_setParent(Process* process, Process* parent) {
    schedule_enterCritical();
    if (parent == NULL) {
        DEBUG_ASSERT_SILENT(process->ppid != 0);
        process->ppid = 0;
        linkedListNode_delete(&process->childProcessNode);
    } else {
        DEBUG_ASSERT_SILENT(process->ppid == 0);
        process->ppid = parent->pid;
        linkedListNode_insertFront(&parent->childProcesses, &process->childProcessNode);
    }
    schedule_leaveCritical();
}

fsEntry* process_getFSentry(Process* process, int fd) {
    schedule_enterCritical();
    Vector* entries = &process->fsEntries;
    fsEntry* ret = entries->size <= fd ? NULL : (fsEntry*)vector_get(entries, fd);
    schedule_leaveCritical();
    return ret;
}

Index32 process_addFSentry(Process* process, fsEntry* entry) {
    schedule_enterCritical();
    Vector* entries = &process->fsEntries;
    Index32 ret = entries->size - 1;

    vector_push(entries, (Object)entry);
    ERROR_GOTO_IF_ERROR(0);

    return ret;
    ERROR_FINAL_BEGIN(0);
    schedule_leaveCritical();
    return INVALID_INDEX32;
}

fsEntry* process_removeFSentry(Process* process, int fd) {
    schedule_enterCritical();
    Vector* entries = &process->fsEntries;

    fsEntry* ret = (fsEntry*)vector_get(entries, fd);
    ERROR_GOTO_IF_ERROR(0);
    vector_set(entries, fd, OBJECT_NULL);

    schedule_leaveCritical();
    
    return ret;
    ERROR_FINAL_BEGIN(0);
    schedule_leaveCritical();
    return NULL;
}