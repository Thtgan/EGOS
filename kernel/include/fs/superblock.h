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
    Result  (*openInode)(SuperBlock* superBlock, iNode* iNodePtr, fsEntryDesc* desc);
    Result  (*closeInode)(SuperBlock* superBlock, iNode* iNode);

    Result  (*openfsEntry)(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags);
    Result  (*closefsEntry)(SuperBlock* superBlock, fsEntry* entry);

    Result  (*create)(SuperBlock* superBlock, fsEntryDesc* descOut, ConstCstring name, ConstCstring parentPath, fsEntryType type, DeviceID deviceID, Flags16 flags);
    Result  (*flush)(SuperBlock* superBlock);

    Result  (*mount)(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc);
    Result  (*unmount)(SuperBlock* superBlock, fsEntryIdentifier* identifier);
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

static inline Result superBlock_rawOpenInode(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc) {
    return superBlock->operations->openInode(superBlock, iNode, desc);
}

static inline Result superBlock_rawCloseInode(SuperBlock* superBlock, iNode* iNode) {
    return superBlock->operations->closeInode(superBlock, iNode);
}

static inline Result superBlock_rawOpenfsEntry(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags) {
    return superBlock->operations->openfsEntry(superBlock, entry, desc, flags);
}

static inline Result superBlock_rawClosefsEntry(SuperBlock* superBlock, fsEntry* entry) {
    return superBlock->operations->closefsEntry(superBlock, entry);
}

static inline Result superBlock_rawCreate(SuperBlock* superBlock, fsEntryDesc* descOut, ConstCstring name, ConstCstring parentPath, fsEntryType type, DeviceID deviceID, Flags16 flags) {
    return superBlock->operations->create(superBlock, descOut, name, parentPath, type, deviceID, flags);
}

static inline Result superBlock_rawMount(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc) {
    return superBlock->operations->mount(superBlock, identifier, targetSuperBlock, targetDesc);
}

static inline Result superBlock_rawUnmount(SuperBlock* superBlock, fsEntryIdentifier* identifier) {
    return superBlock->operations->unmount(superBlock, identifier);
}

static inline Result superBlock_rawFlush(SuperBlock* superBlock) {
    return superBlock->operations->flush(superBlock);
}

void superBlock_initStruct(SuperBlock* superBlock, SuperBlockInitArgs* args);

Result superBlock_getfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock** finalSuperBlockOut, fsEntryDesc** descOut);

Result superBlock_releasefsEntryDesc(SuperBlock* superBlock, fsEntryDesc* entryDesc);

Result superBlock_openfsEntry(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntry* entryOut, FCNTLopenFlags flags);

Result superBlock_closefsEntry(fsEntry* entry);

Result mount_initStruct(Mount* mount, ConstCstring path, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc);

void mount_clearStruct(Mount* mount);

Result superBlock_genericMount(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc);

Result superBlock_genericUnmount(SuperBlock* superBlock, fsEntryIdentifier* identifier);

DirMountList* superBlock_getDirMountList(SuperBlock* superBlock, ConstCstring directoryPath);

Result superBlock_stepSearchMount(SuperBlock* superBlock, String* searchPath, Index64* searchPathSplitRet, SuperBlock** newSuperBlockRet, fsEntryDesc** mountedToDescRet);

#endif // __FS_SUPERBLOCK_H
