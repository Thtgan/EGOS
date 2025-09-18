#if !defined(__FS_VNODE_H)
#define __FS_VNODE_H

typedef struct vNodeAttribute vNodeAttribute;
typedef struct vNode vNode;
typedef struct vNodeOperations vNodeOperations;

#include<fs/directoryEntry.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/fscore.h>
#include<kit/types.h>
#include<multitask/locks/spinlock.h>
#include<structs/hashTable.h>
#include<structs/refCounter.h>
#include<structs/singlyLinkedList.h>

typedef struct vNodeAttribute {
    Uint32 uid; //TODO: Not used yet
    Uint32 gid; //TODO: Not used yet
    Uint64 createTime;
    Uint64 lastAccessTime;
    Uint64 lastModifyTime;
} vNodeAttribute;

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

    vNodeAttribute          attribute;

    ID                      deviceID;

    Spinlock                lock;   //TODO: Use mutex?
} vNode;

typedef struct vNodeOperations {
    void (*readData)(vNode* vnode, Index64 begin, void* buffer, Size byteN);

    void (*writeData)(vNode* vnode, Index64 begin, const void* buffer, Size byteN);

    void (*resize)(vNode* vnode, Size newSizeInByte);   //TODO: Add Sync

    void (*readAttr)(vNode* vnode, vNodeAttribute* attribute);

    void (*writeAttr)(vNode* vnode, vNodeAttribute* attribute);
    //=========== Directory Functions ===========
    Index64 (*addDirectoryEntry)(vNode* vnode, DirectoryEntry* entry, vNodeAttribute* attr);

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

static inline void vNode_rawReadAttr(vNode* vnode, vNodeAttribute* attribute) {
    vnode->operations->readAttr(vnode, attribute);
}

static inline void vNode_rawWriteAttr(vNode* vnode, vNodeAttribute* attribute) {
    vnode->operations->writeAttr(vnode, attribute);
}

static inline Index64 vNode_rawAddDirectoryEntry(vNode* vnode, DirectoryEntry* entry, vNodeAttribute* attr) {
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

void vNode_addDirectoryEntry(vNode* vnode, DirectoryEntry* entry, vNodeAttribute* attr);

void vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory);

void vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName);

void vNode_genericReadAttr(vNode* vnode, vNodeAttribute* attribute);

void vNode_genericWriteAttr(vNode* vnode, vNodeAttribute* attribute);

static inline Uint32 vNode_getReferenceCount(vNode* vnode) {
    return REF_COUNTER_GET(vnode->refCounter);
}

#endif // __FS_VNODE_H
