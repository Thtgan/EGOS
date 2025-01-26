#if !defined(__FS_SUPERBLOCK_H)
#define __FS_SUPERBLOCK_H

typedef struct SuperBlock SuperBlock;
typedef struct SuperBlockOperations SuperBlockOperations;
typedef struct SuperBlockInitArgs SuperBlockInitArgs;
typedef struct DirMountList DirMountList;
typedef struct Mount Mount;

#include<devices/block/blockDevice.h>
#include<fs/fcntl.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<kit/types.h>
#include<structs/hashTable.h>
#include<structs/string.h>

typedef struct SuperBlock {
    BlockDevice*            blockDevice;
    SuperBlockOperations*   operations;

    fsEntryDesc*            rootDirDesc;
    void*                   specificInfo;
    HashTable               openedInode;
    HashTable               mounted;
    HashTable               fsEntryDescs;
} SuperBlock;

typedef struct SuperBlockOperations {
    void  (*openInode)(SuperBlock* superBlock, iNode* iNodePtr, fsEntryDesc* desc);
    void  (*closeInode)(SuperBlock* superBlock, iNode* iNode);

    void  (*openfsEntry)(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags);
    void  (*closefsEntry)(SuperBlock* superBlock, fsEntry* entry);

    void  (*create)(SuperBlock* superBlock, fsEntryDesc* descOut, ConstCstring name, ConstCstring parentPath, fsEntryType type, DeviceID deviceID, Flags16 flags);
    void  (*flush)(SuperBlock* superBlock);

    void  (*mount)(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc);
    void  (*unmount)(SuperBlock* superBlock, fsEntryIdentifier* identifier);
} SuperBlockOperations;

typedef struct SuperBlockInitArgs {
    BlockDevice*            blockDevice;
    SuperBlockOperations*   operations;
    fsEntryDesc*            rootDirDesc;
    void*                   specificInfo;
    Size                    openedInodeBucket;
    SinglyLinkedList*       openedInodeChains;
    Size                    mountedBucket;
    SinglyLinkedList*       mountedChains;
    Size                    fsEntryDescBucket;
    SinglyLinkedList*       fsEntryDescChains;
} SuperBlockInitArgs;

//Contains all mounted descriptors in a directory
typedef struct DirMountList {
    HashChainNode   node;
    LinkedList      list;
} DirMountList;

//Stands for a mount instance in directory
typedef struct Mount {
    String          name;
    LinkedListNode  node;
    SuperBlock*     superblock;
    fsEntryDesc*    targetDesc;
} Mount;

static inline void superBlock_rawOpenInode(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc) {
    superBlock->operations->openInode(superBlock, iNode, desc);
}

static inline void superBlock_rawCloseInode(SuperBlock* superBlock, iNode* iNode) {
    superBlock->operations->closeInode(superBlock, iNode);
}

static inline void superBlock_rawOpenfsEntry(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags) {
    superBlock->operations->openfsEntry(superBlock, entry, desc, flags);
}

static inline void superBlock_rawClosefsEntry(SuperBlock* superBlock, fsEntry* entry) {
    superBlock->operations->closefsEntry(superBlock, entry);
}

static inline void superBlock_rawCreate(SuperBlock* superBlock, fsEntryDesc* descOut, ConstCstring name, ConstCstring parentPath, fsEntryType type, DeviceID deviceID, Flags16 flags) {
    superBlock->operations->create(superBlock, descOut, name, parentPath, type, deviceID, flags);
}

static inline void superBlock_rawMount(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc) {
    superBlock->operations->mount(superBlock, identifier, targetSuperBlock, targetDesc);
}

static inline void superBlock_rawUnmount(SuperBlock* superBlock, fsEntryIdentifier* identifier) {
    superBlock->operations->unmount(superBlock, identifier);
}

static inline void superBlock_rawFlush(SuperBlock* superBlock) {
    superBlock->operations->flush(superBlock);
}

void superBlock_initStruct(SuperBlock* superBlock, SuperBlockInitArgs* args);

void superBlock_getfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock** finalSuperBlockOut, fsEntryDesc** descOut);

void superBlock_releasefsEntryDesc(SuperBlock* superBlock, fsEntryDesc* entryDesc);

void superBlock_openfsEntry(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntry* entryOut, FCNTLopenFlags flags);

void superBlock_closefsEntry(fsEntry* entry);

void mount_initStruct(Mount* mount, ConstCstring path, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc);

void mount_clearStruct(Mount* mount);

void superBlock_genericMount(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc);

void superBlock_genericUnmount(SuperBlock* superBlock, fsEntryIdentifier* identifier);

DirMountList* superBlock_getDirMountList(SuperBlock* superBlock, ConstCstring directoryPath);

bool superBlock_stepSearchMount(SuperBlock* superBlock, String* searchPath, Index64* searchPathSplitRet, SuperBlock** newSuperBlockRet, fsEntryDesc** mountedToDescRet);

#endif // __FS_SUPERBLOCK_H
