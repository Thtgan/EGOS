#include<multitask/pipe.h>

typedef struct PipeVnode PipeVnode;

#include<devices/device.h>
#include<fs/fcntl.h>
#include<fs/fsEntry.h>
#include<fs/fscore.h>
#include<fs/fsNode.h>
#include<fs/fcntl.h>
#include<fs/vnode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/mm.h>
#include<memory/memory.h>
#include<multitask/locks/conditionVar.h>
#include<multitask/locks/mutex.h>
#include<structs/fifo.h>
#include<structs/hashTable.h>
#include<structs/refCounter.h>
#include<algorithms.h>
#include<error.h>

typedef struct PipeVnode {
    vNode vnode;
    Pipe* pipe;
} PipeVnode;

static ConstCstring _pipe_fsEntry_name = "pipe";

static void __pipe_initStruct(Pipe* pipe);

static void __pipe_clearStruct(Pipe* pipe);

static bool __pipe_isReadyToRead(void* args);

static void __pipeVnode_readData(vNode* vnode, Index64 begin, void* buffer, Size byteN);

static void __pipeVnode_writeData(vNode* vnode, Index64 begin, const void* buffer, Size byteN);

static Index64 __pipe_fsEntry_genericSeek(fsEntry* entry, Index64 seekTo);

static vNode* __pipe_fscore_openVnode(FScore* fscore, fsNode* node);

static void __pipe_fscore_closeVnode(FScore* fscore, vNode* vnode);

static fsEntry* __pipe_fscore_openFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags);

static vNodeOperations _pipe_vnode_operations = {
    .readData = __pipeVnode_readData,
    .writeData = __pipeVnode_writeData,
    .resize = NULL,
    .addDirectoryEntry = NULL,
    .removeDirectoryEntry = NULL,
    .renameDirectoryEntry = NULL,
    .readDirectoryEntries = NULL
};

static fsEntryOperations _pipe_fsEntry_operations = {
    .seek   = __pipe_fsEntry_genericSeek,
    .read   = fsEntry_genericRead,
    .write  = fsEntry_genericWrite
};

static FScoreOperations _pipe_fscore_oeprations = {
    .openVnode = __pipe_fscore_openVnode,
    .closeVnode = __pipe_fscore_closeVnode,
    .sync = NULL,
    .openFSentry = __pipe_fscore_openFSentry,
    .closeFSentry = fscore_genericCloseFSentry,
    .mount = NULL,
    .unmount = NULL
};

static FScore _pipe_fscore = {
    .blockDevice = NULL,
    .rootFSnode = NULL,
    .operations = &_pipe_fscore_oeprations
};

//TODO: What a mess

Pipe* pipe_create(fsEntry** writeEndRet, fsEntry** readEndRet) {
    Pipe* ret = mm_allocate(sizeof(Pipe));
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    __pipe_initStruct(ret);
    pipe_getFSentries(ret, writeEndRet, readEndRet);
    DEBUG_ASSERT_SILENT(ret->vnode != NULL);
    
    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void pipe_getFSentries(Pipe* pipe, fsEntry** writeEndRet, fsEntry** readEndRet) {
    vNode* vnode = fscore_getVnode(&_pipe_fscore, pipe->dummyNode, false);
    *writeEndRet = fscore_rawOpenFSentry(&_pipe_fscore, vnode, FCNTL_OPEN_WRITE_ONLY);
    
    vnode = fscore_getVnode(&_pipe_fscore, pipe->dummyNode, false);
    *readEndRet = fscore_rawOpenFSentry(&_pipe_fscore, vnode, FCNTL_OPEN_READ_ONLY);
}

static void __pipe_initStruct(Pipe* pipe) {
    fifo_initStruct(&pipe->fifo);

    DirectoryEntry entry;
    memory_memset(&entry, 0, sizeof(DirectoryEntry));
    entry.name = _pipe_fsEntry_name;
    entry.pointsTo = (Uint64)pipe;
    entry.type = FS_ENTRY_TYPE_PIPE;
    entry.vnodeID = INVALID_ID;
    
    FSnodeAttribute attribute;
    memory_memset(&attribute, 0, sizeof(FSnodeAttribute));
    
    pipe->dummyNode = fsnode_create(&entry, 0, &attribute, NULL);
    pipe->vnode = NULL;

    pipe->dataByteN = 0;
    mutex_initStruct(&pipe->lock, EMPTY_FLAGS);
    conditionVar_initStruct(&pipe->cond);
}

static void __pipe_clearStruct(Pipe* pipe) {
    DEBUG_ASSERT_SILENT(pipe->vnode == NULL);
    fsnode_derefer(pipe->dummyNode);
}

static bool __pipe_isReadyToRead(void* args) {
    return ((Pipe*)args)->dataByteN > 0;
}

static void __pipeVnode_readData(vNode* vnode, Index64 begin, void* buffer, Size byteN) {   //TODO: Read EOF when all writers closed
    PipeVnode* pipeVnode = HOST_POINTER(vnode, PipeVnode, vnode);
    Pipe* pipe = pipeVnode->pipe;

    mutex_acquire(&pipe->lock);

    conditionVar_wait(&pipe->cond, &pipe->lock, __pipe_isReadyToRead, (void*)pipe);

    byteN = algorithms_umin64(byteN, pipe->dataByteN);
    
    fifo_read(&pipe->fifo, buffer, byteN);

    pipe->dataByteN -= byteN;
    if (__pipe_isReadyToRead(pipe)) {
        conditionVar_notify(&pipe->cond);
    }

    mutex_release(&pipe->lock);
}

static void __pipeVnode_writeData(vNode* vnode, Index64 begin, const void* buffer, Size byteN) {
    PipeVnode* pipeVnode = HOST_POINTER(vnode, PipeVnode, vnode);
    Pipe* pipe = pipeVnode->pipe;

    mutex_acquire(&pipe->lock);
    
    fifo_write(&pipe->fifo, buffer, byteN);

    pipe->dataByteN += byteN;
    
    mutex_release(&pipe->lock);

    conditionVar_notify(&pipe->cond);
}

static Index64 __pipe_fsEntry_genericSeek(fsEntry* entry, Index64 seekTo) {
    return 0;
}

static vNode* __pipe_fscore_openVnode(FScore* fscore, fsNode* node) {
    PipeVnode* pipeVnode = mm_allocate(sizeof(PipeVnode));
    vNode* vnode = &pipeVnode->vnode;

    vNodeInitArgs args = {
        .vnodeID = INVALID_ID,
        .tokenSpaceSize = INFINITE,
        .size = INFINITE,
        .fscore = &_pipe_fscore,
        .operations = &_pipe_vnode_operations,
        .fsNode = node,
        .deviceID = DEVICE_INVALID_ID
    };

    vNode_initStruct(vnode, &args);
    pipeVnode->pipe = (Pipe*)node->entry.pointsTo;
    
    pipeVnode->pipe->vnode = vnode;

    return vnode;
}

static void __pipe_fscore_closeVnode(FScore* fscore, vNode* vnode) {
    DEBUG_ASSERT_SILENT(REF_COUNTER_GET(vnode->refCounter) == 0);
    PipeVnode* pipeVnode = mm_allocate(sizeof(PipeVnode));
    Pipe* pipe = pipeVnode->pipe;
    mm_free(pipeVnode);

    __pipe_clearStruct(pipe);
}

static fsEntry* __pipe_fscore_openFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags) {
    fsEntry* ret = fscore_genericOpenFSentry(fscore, vnode, flags);
    ERROR_GOTO_IF_ERROR(0);

    ret->operations = &_pipe_fsEntry_operations;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}