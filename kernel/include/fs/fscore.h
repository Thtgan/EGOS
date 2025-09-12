#if !defined(__FS_FSCORE_H)
#define __FS_FSCORE_H

typedef struct FScore FScore;
typedef struct FScoreOperations FScoreOperations;
typedef struct FScoreInitArgs FScoreInitArgs;
typedef struct Mount Mount;

#include<devices/blockDevice.h>
#include<fs/fcntl.h>
#include<fs/fsEntry.h>
#include<fs/fsIdentifier.h>
#include<fs/fsNode.h>
#include<fs/vnode.h>
#include<kit/atomic.h>
#include<kit/types.h>
#include<structs/RBtree.h>
#include<structs/string.h>

typedef struct FScore {
    BlockDevice*        blockDevice;
    FScoreOperations*   operations;

    fsNode*             rootFSnode;
    LinkedList          mounted;
} FScore;

typedef struct FScoreOperations {
    vNode* (*openVnode)(FScore* fscore, fsNode* node);
    void (*closeVnode)(FScore* fscore, vNode* vnode);

    void (*sync)(FScore* fscore);

    fsEntry* (*openFSentry)(FScore* fscore, vNode* vnode, FCNTLopenFlags flags);
    void (*closeFSentry)(FScore* fscore, fsEntry* entry);

    void (*mount)(FScore* fscore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags);
    void (*unmount)(FScore* fscore, fsIdentifier* mountPoint);
} FScoreOperations;

typedef struct FScoreInitArgs {
    BlockDevice*        blockDevice;
    FScoreOperations*   operations;
    Index64             rootFSnodePointsTo;
} FScoreInitArgs;

//Stands for a mount instance in directory
typedef struct Mount {
    String          path;
    LinkedListNode  node;
    vNode*          mountedVnode;
} Mount;

static inline vNode* fscore_rawOpenVnode(FScore* fscore, fsNode* node) {
    return fscore->operations->openVnode(fscore, node);
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

fsNode* fscore_getFSnode(FScore* fscore, fsIdentifier* identifier, FScore** finalFScoreOut, bool followMount);

void fscore_releaseFSnode(fsNode* node);

vNode* fscore_getVnode(FScore* fscore, fsNode* node, bool followMount);

void fscore_releaseVnode(vNode* vnode);

fsEntry* fscore_genericOpenFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags);

void fscore_genericCloseFSentry(FScore* fscore, fsEntry* entry);

void fscore_genericMount(FScore* fscore, fsIdentifier* mountPoint, vNode* mountVnode, Flags8 flags);

void fscore_genericUnmount(FScore* fscore, fsIdentifier* mountPoint);

Mount* fscore_lookupMount(FScore* fscore, ConstCstring path);

#endif // __FS_FSCORE_H
