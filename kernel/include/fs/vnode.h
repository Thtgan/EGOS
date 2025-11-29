#if !defined(__FS_VNODE_H)
#define __FS_VNODE_H

typedef struct vNode vNode;
typedef struct vNodeOperations vNodeOperations;
typedef struct vNodeInitArgs vNodeInitArgs;

#include<fs/directoryEntry.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/fscore.h>
#include<kit/types.h>
#include<multitask/locks/spinlock.h>
#include<structs/hashTable.h>
#include<structs/refCounter.h>
#include<structs/singlyLinkedList.h>

typedef struct vNode {
    Uint32                  signature;
#define VNODE_SIGNATURE     0x120DE516
    ID                      vnodeID;
    Size                    tokenSpaceSize;
    Size                    size;
    FScore*                 fscore;
    vNodeOperations*        operations;

    RefCounter32            refCounter;
    HashChainNode           openedNode;
    fsNode*                 fsNode;

    ID                      deviceID;

    Spinlock                lock;   //TODO: Use mutex?
} vNode;

typedef struct vNodeInitArgs {
    ID vnodeID;
    Size tokenSpaceSize;
    Size size;
    FScore* fscore;
    vNodeOperations* operations;
    fsNode* fsNode;
    ID deviceID;
} vNodeInitArgs;

typedef struct vNodeOperations {
    void (*readData)(vNode* vnode, Index64 begin, void* buffer, Size byteN);

    void (*writeData)(vNode* vnode, Index64 begin, const void* buffer, Size byteN);

    void (*resize)(vNode* vnode, Size newSizeInByte);   //TODO: Add Sync
    //=========== Directory Functions ===========
    Index64 (*addDirectoryEntry)(vNode* vnode, DirectoryEntry* entry, FSnodeAttribute* attr);

    void (*removeDirectoryEntry)(vNode* vnode, ConstCstring name, bool isDirectory);

    void (*renameDirectoryEntry)(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName);

    void (*readDirectoryEntries)(vNode* vnode);
} vNodeOperations;

static inline void vNode_rawReadData(vNode* vnode, Index64 begin, void* buffer, Size byteN) {
    vnode->operations->readData(vnode, begin, buffer, byteN);
}

static inline void vNode_rawWriteData(vNode* vnode, Index64 begin, const void* buffer, Size byteN) {
    vnode->operations->writeData(vnode, begin, buffer, byteN);
}

static inline void vNode_rawResize(vNode* vnode, Size newSizeInByte) {
    vnode->operations->resize(vnode, newSizeInByte);
}

static inline Index64 vNode_rawAddDirectoryEntry(vNode* vnode, DirectoryEntry* entry, FSnodeAttribute* attr) {
    return vnode->operations->addDirectoryEntry(vnode, entry, attr);
}

static inline void vNode_rawRemoveDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory) {
    vnode->operations->removeDirectoryEntry(vnode, name, isDirectory);
}

static inline void vNode_rawRenameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName) {
    vnode->operations->renameDirectoryEntry(vnode, entry, moveTo, newName);
}

static inline void vNode_rawReadDirectoryEntries(vNode* vnode) {
    vnode->operations->readDirectoryEntries(vnode);
}

void vNode_initStruct(vNode* vnode, vNodeInitArgs* args);

void vNode_addDirectoryEntry(vNode* vnode, DirectoryEntry* entry, FSnodeAttribute* attr);

void vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory);

void vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName);

static inline Uint32 vNode_getReferenceCount(vNode* vnode) {
    return REF_COUNTER_GET(vnode->refCounter);
}

#endif // __FS_VNODE_H
