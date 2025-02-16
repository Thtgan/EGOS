#if !defined(__FS_SUPERBLOCK_H)
#define __FS_SUPERBLOCK_H

typedef struct SuperBlock SuperBlock;
typedef struct SuperBlockOperations SuperBlockOperations;
typedef struct SuperBlockInitArgs SuperBlockInitArgs;
typedef struct Mount Mount;

#include<devices/block/blockDevice.h>
#include<fs/fcntl.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<kit/types.h>
#include<structs/hashTable.h>
#include<structs/string.h>

#include<kit/atomic.h>
#include<fs/fsNode.h>
#include<fs/fsIdentifier.h>

typedef struct SuperBlock {
    BlockDevice*            blockDevice;
    SuperBlockOperations*   operations;

    iNode*                  rootInode;

    HashTable               openedInode;
    LinkedList              mounted;

    ID                      nextInodeID;
} SuperBlock;

typedef struct SuperBlockOperations {
    iNode* (*openInode)(SuperBlock* superBlock, ID inodeID);
    iNode* (*openRootInode)(SuperBlock* superBlock);
    void (*closeInode)(SuperBlock* superBlock, iNode* inode);

    void (*sync)(SuperBlock* superBlock);

    fsEntry* (*openFSentry)(SuperBlock* superBlock, iNode* inode, FCNTLopenFlags flags);
    void (*closeFSentry)(SuperBlock* superBlock, fsEntry* entry);

    void (*mount)(SuperBlock* superBlock, fsIdentifier* mountPoint, iNode* mountInode, Flags8 flags);
    void (*unmount)(SuperBlock* superBlock, fsIdentifier* mountPoint);
} SuperBlockOperations;

typedef struct SuperBlockInitArgs {
    BlockDevice*            blockDevice;
    SuperBlockOperations*   operations;
    Size                    openedInodeBucket;
    SinglyLinkedList*       openedInodeChains;
} SuperBlockInitArgs;

//Stands for a mount instance in directory
typedef struct Mount {
    String          path;
    LinkedListNode  node;
    iNode*          mountedInode;
} Mount;

static inline iNode* superBlock_rawOpenInode(SuperBlock* superBlock, ID inodeID) {
    return superBlock->operations->openInode(superBlock, inodeID);
}

static inline iNode* superBlock_rawOpenRootInode(SuperBlock* superBlock) {
    return superBlock->operations->openRootInode(superBlock);
}

static inline void superBlock_rawCloseInode(SuperBlock* superBlock, iNode* inode) {
    superBlock->operations->closeInode(superBlock, inode);
}

static inline void superBlock_rawSync(SuperBlock* superBlock) {
    superBlock->operations->sync(superBlock);
}

static inline fsEntry* superBlock_rawOpenFSentry(SuperBlock* superBlock, iNode* inode, FCNTLopenFlags flags) {
    return superBlock->operations->openFSentry(superBlock, inode, flags);
}

static inline void superBlock_rawCloseFSentry(SuperBlock* superBlock, fsEntry* entry) {
    superBlock->operations->closeFSentry(superBlock, entry);
}

static inline void superBlock_rawMount(SuperBlock* superBlock, fsIdentifier* mountPoint, iNode* mountInode, Flags8 flags) {
    superBlock->operations->mount(superBlock, mountPoint, mountInode, flags);
}

static inline void superBlock_rawUnmount(SuperBlock* superBlock, fsIdentifier* mountPoint) {
    superBlock->operations->unmount(superBlock, mountPoint);
}

void superBlock_initStruct(SuperBlock* superBlock, SuperBlockInitArgs* args);

ID superBlock_allocateInodeID(SuperBlock* superBlock);

iNode* superBlock_openInode(SuperBlock* superBlock, ID inodeID);

void superBlock_closeInode(iNode* inode);

fsEntry* superBlock_genericOpenFSentry(SuperBlock* superBlock, iNode* inode, FCNTLopenFlags flags);

void superBlock_genericCloseFSentry(SuperBlock* superBlock, fsEntry* entry);

void superBlock_genericMount(SuperBlock* superBlock, fsIdentifier* mountPoint, iNode* mountInode, Flags8 flags);

void superBlock_genericUnmount(SuperBlock* superBlock, fsIdentifier* mountPoint);

Mount* superBlock_lookupMount(SuperBlock* superBlock, ConstCstring path);

#endif // __FS_SUPERBLOCK_H
