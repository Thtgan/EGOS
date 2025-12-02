#if !defined(__MULTITASK_PIPE_H)
#define __MULTITASK_PIPE_H

#include<fs/fsEntry.h>
#include<fs/vnode.h>
#include<fs/fsNode.h>
#include<multitask/locks/conditionVar.h>
#include<multitask/locks/mutex.h>
#include<structs/fifo.h>
#include<structs/refCounter.h>

typedef struct Pipe Pipe;

typedef struct Pipe {
    FIFO fifo;
    fsNode* dummyNode;
    vNode* vnode;
    Size dataByteN;
    Mutex lock;
    ConditionVar cond;
} Pipe;

// void pipe_initStruct(Pipe* pipe);

// void pipe_clearStruct(Pipe* pipe);

Pipe* pipe_create(fsEntry** writeEndRet, fsEntry** readEndRet); //The only way to release it is to release to fsEntries

void pipe_getFSentries(Pipe* pipe, fsEntry** writeEndRet, fsEntry** readEndRet);

#endif // __MULTITASK_PIPE_H
