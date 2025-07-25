#if !defined(__FS_FSCORE_H)
#define __FS_FSCORE_H

typedef struct FScore FScore;
typedef struct FScoreOperations FScoreOperations;
typedef struct FScoreInitArgs FScoreInitArgs;
typedef struct Mount Mount;

#include<devices/blockDevice.h>
#include<fs/fcntl.h>
#include<fs/fsEntry.h>
#include<fs/vnode.h>
#include<kit/types.h>
#include<structs/hashTable.h>
#include<structs/string.h>

#include<kit/atomic.h>
#include<fs/fsNode.h>
#include<fs/fsIdentifier.h>

typedef struct FScore {
    BlockDevice*            blockDevice;
    FScoreOperations*   operations;

    vNode*                  rootVnode;

    HashTable               openedVnode;
    LinkedList              mounted;

    ID                      nextVnodeID;
} FScore;

typedef struct FScoreOperations {
    fsNode* (*getFSnode)(FScore* fsCore, ID vnodeID);

    vNode* (*openVnode)(FScore* fsCore, ID vnodeID);
    vNode* (*openRootVnode)(FScore* fsCore);
    void (*closeVnode)(FScore* fsCore, vNode* vnode);

    void (*sync)(FScore* fsCore);

    fsEntry* (*openFSentry)(FScore* fsCore, vNode* vnode, FCNTLopenFlags flags);
    void (*closeFSentry)(FScore* fsCore, fsEntry* entry);

    void (*mount)(FScore* fsCore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags);
    void (*unmount)(FScore* fsCore, fsIdentifier* mountPoint);
} FScoreOperations;

typedef struct FScoreInitArgs {
    BlockDevice*            blockDevice;
    FScoreOperations*   operations;
    Size                    openedVnodeBucket;
    SinglyLinkedList*       openedVnodeChains;
} FScoreInitArgs;

//Stands for a mount instance in directory
typedef struct Mount {
    String          path;
    LinkedListNode  node;
    vNode*          mountedVnode;
} Mount;

static inline fsNode* fsCore_rawGetFSnode(FScore* fsCore, ID vnodeID) {
    return fsCore->operations->getFSnode(fsCore, vnodeID);
}

static inline vNode* fsCore_rawOpenVnode(FScore* fsCore, ID vnodeID) {
    return fsCore->operations->openVnode(fsCore, vnodeID);
}

static inline vNode* fsCore_rawOpenRootVnode(FScore* fsCore) {
    return fsCore->operations->openRootVnode(fsCore);
}

static inline void fsCore_rawCloseVnode(FScore* fsCore, vNode* vnode) {
    fsCore->operations->closeVnode(fsCore, vnode);
}

static inline void fsCore_rawSync(FScore* fsCore) {
    fsCore->operations->sync(fsCore);
}

static inline fsEntry* fsCore_rawOpenFSentry(FScore* fsCore, vNode* vnode, FCNTLopenFlags flags) {
    return fsCore->operations->openFSentry(fsCore, vnode, flags);
}

static inline void fsCore_rawCloseFSentry(FScore* fsCore, fsEntry* entry) {
    fsCore->operations->closeFSentry(fsCore, entry);
}

static inline void fsCore_rawMount(FScore* fsCore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags) {
    fsCore->operations->mount(fsCore, mountPoint, mountVnode, flags);
}

static inline void fsCore_rawUnmount(FScore* fsCore, fsIdentifier* mountPoint) {
    fsCore->operations->unmount(fsCore, mountPoint);
}

void fsCore_initStruct(FScore* fsCore, FScoreInitArgs* args);

ID fsCore_allocateVnodeID(FScore* fsCore);

fsNode* fsCore_getFSnode(FScore* fsCore, ID vnodeID);

vNode* fsCore_openVnode(FScore* fsCore, ID vnodeID);

void fsCore_closeVnode(vNode* vnode);

fsEntry* fsCore_genericOpenFSentry(FScore* fsCore, vNode* vnode, FCNTLopenFlags flags);

void fsCore_genericCloseFSentry(FScore* fsCore, fsEntry* entry);

void fsCore_genericMount(FScore* fsCore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags);

void fsCore_genericUnmount(FScore* fsCore, fsIdentifier* mountPoint);

Mount* fsCore_lookupMount(FScore* fsCore, ConstCstring path);

#endif // __FS_FSCORE_H
