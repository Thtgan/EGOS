#if !defined(__FS_INODE_H)
#define __FS_INODE_H

typedef struct iNodeAttribute iNodeAttribute;
typedef struct iNode iNode;
typedef struct iNodeOperations iNodeOperations;

#include<fs/directoryEntry.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<multitask/locks/spinlock.h>
#include<structs/hashTable.h>
#include<structs/refCounter.h>
#include<structs/singlyLinkedList.h>

typedef struct iNodeAttribute {
    Uint32 uid; //TODO: Not used yet
    Uint32 gid; //TODO: Not used yet
    Uint64 createTime;
    Uint64 lastAccessTime;
    Uint64 lastModifyTime;
} iNodeAttribute;

typedef struct iNode {
    Uint32                  signature;
#define INODE_SIGNATURE     0x120DE516
    ID                      inodeID;
    Size                    sizeInBlock;
    Size                    sizeInByte;
    SuperBlock*             superBlock;
    iNodeOperations*        operations;

    RefCounter              refCounter;
    HashChainNode           openedNode;
    fsNode*                 fsNode;

    iNodeAttribute          attribute;

    ID                      deviceID;

    Spinlock                lock;   //TODO: Use mutex?
} iNode;

typedef bool (*iNodeOperationIterateDirectoryEntryFunc)(iNode* inode, DirectoryEntry* entry, Object arg, void* ret);  //TODO: Add mode

typedef struct iNodeOperations {
    void (*readData)(iNode* inode, Index64 begin, void* buffer, Size byteN);

    void (*writeData)(iNode* inode, Index64 begin, const void* buffer, Size byteN);

    void (*resize)(iNode* inode, Size newSizeInByte);   //TODO: Add Sync

    void (*readAttr)(iNode* inode, iNodeAttribute* attribute);

    void (*writeAttr)(iNode* inode, iNodeAttribute* attribute);
    //=========== Directory Functions ===========
    void (*iterateDirectoryEntries)(iNode* inode, iNodeOperationIterateDirectoryEntryFunc func, Object arg, void* ret);

    // fsNode* (*lookupDirectoryEntry)(iNode* inode, ConstCstring name, bool isDirectory);

    void (*addDirectoryEntry)(iNode* inode, ConstCstring name, fsEntryType type, iNodeAttribute* attr, ID deviceID);

    void (*removeDirectoryEntry)(iNode* inode, ConstCstring name, bool isDirectory);

    void (*renameDirectoryEntry)(iNode* inode, fsNode* entry, iNode* moveTo, ConstCstring newName);
} iNodeOperations;

static inline void iNode_rawReadData(iNode* inode, Index64 begin, void* buffer, Size byteN) {
    inode->operations->readData(inode, begin, buffer, byteN);
}

static inline void iNode_rawWriteData(iNode* inode, Index64 begin, const void* buffer, Size byteN) {
    inode->operations->writeData(inode, begin, buffer, byteN);
}

static inline void iNode_rawResize(iNode* inode, Size newSizeInByte) {
    inode->operations->resize(inode, newSizeInByte);
}

static inline void iNode_rawReadAttr(iNode* inode, iNodeAttribute* attribute) {
    inode->operations->readAttr(inode, attribute);
}

static inline void iNode_rawWriteAttr(iNode* inode, iNodeAttribute* attribute) {
    inode->operations->writeAttr(inode, attribute);
}

static inline void iNode_rawIterateDirectoryEntries(iNode* inode, iNodeOperationIterateDirectoryEntryFunc func, Object arg, void* ret) {
    inode->operations->iterateDirectoryEntries(inode, func, arg, ret);
}

static inline void iNode_rawAddDirectoryEntry(iNode* inode, ConstCstring name, fsEntryType type, iNodeAttribute* attr, ID deviceID) {
    inode->operations->addDirectoryEntry(inode, name, type, attr, deviceID);
}

static inline void iNode_rawRemoveDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory) {
    inode->operations->removeDirectoryEntry(inode, name, isDirectory);
}

static inline void iNode_rawRenameDirectoryEntry(iNode* inode, fsNode* entry, iNode* moveTo, ConstCstring newName) {
    inode->operations->renameDirectoryEntry(inode, entry, moveTo, newName);
}

fsNode* iNode_lookupDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory);

void iNode_removeDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory);

void iNode_genericReadAttr(iNode* inode, iNodeAttribute* attribute);

void iNode_genericWriteAttr(iNode* inode, iNodeAttribute* attribute);

static inline Uint32 iNode_getReferenceCount(iNode* inode) {
    return refCounter_getCount(&inode->refCounter);
}

#endif // __FS_INODE_H
