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
    fsNode* (*getFSnode)(FScore* fscore, ID vnodeID);

    vNode* (*openVnode)(FScore* fscore, ID vnodeID);
    vNode* (*openRootVnode)(FScore* fscore);
    void (*closeVnode)(FScore* fscore, vNode* vnode);

    void (*sync)(FScore* fscore);

    fsEntry* (*openFSentry)(FScore* fscore, vNode* vnode, FCNTLopenFlags flags);
    void (*closeFSentry)(FScore* fscore, fsEntry* entry);

    void (*mount)(FScore* fscore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags);
    void (*unmount)(FScore* fscore, fsIdentifier* mountPoint);
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

static inline fsNode* fscore_rawGetFSnode(FScore* fscore, ID vnodeID) {
    return fscore->operations->getFSnode(fscore, vnodeID);
}

static inline vNode* fscore_rawOpenVnode(FScore* fscore, ID vnodeID) {
    return fscore->operations->openVnode(fscore, vnodeID);
}

static inline vNode* fscore_rawOpenRootVnode(FScore* fscore) {
    return fscore->operations->openRootVnode(fscore);
}

static inline void fscore_rawCloseVnode(FScore* fscore, vNode* vnode) {
    fscore->operations->closeVnode(fscore, vnode);
}

static inline void fscore_rawSync(FScore* fscore) {
    fscore->operations->sync(fscore);
}

static inline fsEntry* fscore_rawOpenFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags) {
    return fscore->operations->openFSentry(fscore, vnode, flags);
}

static inline void fscore_rawCloseFSentry(FScore* fscore, fsEntry* entry) {
    fscore->operations->closeFSentry(fscore, entry);
}

static inline void fscore_rawMount(FScore* fscore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags) {
    fscore->operations->mount(fscore, mountPoint, mountVnode, flags);
}

static inline void fscore_rawUnmount(FScore* fscore, fsIdentifier* mountPoint) {
    fscore->operations->unmount(fscore, mountPoint);
}

void fscore_initStruct(FScore* fscore, FScoreInitArgs* args);

ID fscore_allocateVnodeID(FScore* fscore);

fsNode* fscore_getFSnode(FScore* fscore, ID vnodeID);

vNode* fscore_openVnode(FScore* fscore, ID vnodeID);

void fscore_closeVnode(vNode* vnode);

fsEntry* fscore_genericOpenFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags);

void fscore_genericCloseFSentry(FScore* fscore, fsEntry* entry);

void fscore_genericMount(FScore* fscore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags);

void fscore_genericUnmount(FScore* fscore, fsIdentifier* mountPoint);

Mount* fscore_lookupMount(FScore* fscore, ConstCstring path);

#endif // __FS_FSCORE_H
