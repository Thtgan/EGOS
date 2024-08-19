#if !defined(__FS_STRUCT_H)
#define __FS_STRUCT_H

#include<devices/block/blockDevice.h>
#include<devices/device.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/hashTable.h>
#include<structs/linkedList.h>
#include<structs/singlyLinkedList.h>
#include<structs/string.h>

STRUCT_PRE_DEFINE(SuperBlock);
STRUCT_PRE_DEFINE(iNode);
STRUCT_PRE_DEFINE(fsEntry);
STRUCT_PRE_DEFINE(fsEntryDesc);
STRUCT_PRE_DEFINE(fsEntryIdentifier);

typedef fsEntry Directory;
typedef fsEntry File;

typedef enum {
    FS_TYPE_FAT32,
    FS_TYPE_DEVFS,
    FS_TYPE_NUM,
    FS_TYPE_UNKNOWN
} FStype;

typedef struct {
    SuperBlock*     superBlock;
    ConstCstring    name;
    FStype          type;
} FS;

typedef struct {
    Result  (*openInode)(SuperBlock* superBlock, iNode* iNodePtr, fsEntryDesc* desc);
    Result  (*closeInode)(SuperBlock* superBlock, iNode* iNode);

    Result  (*openfsEntry)(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc);
    Result  (*closefsEntry)(SuperBlock* superBlock, fsEntry* entry);

    Result  (*mount)(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc);
    Result  (*unmount)(SuperBlock* superBlock, fsEntryIdentifier* identifier);
} SuperBlockOperations;

typedef struct {
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
typedef struct {
    HashChainNode   node;
    LinkedList      list;
} DirMountList;

//Stands for a mount instance in directory
typedef struct {
    String          name;
    LinkedListNode  node;
    SuperBlock*     superblock;
    fsEntryDesc*    targetDesc;
} Mount;

STRUCT_PRIVATE_DEFINE(SuperBlock) { //TODO: Try fix this with a file with pre-defines
    BlockDevice*            blockDevice;
    SuperBlockOperations*   operations;

    fsEntryDesc*            rootDirDesc;
    void*                   specificInfo;
    HashTable               openedInode;
    HashTable               mounted;
    HashTable               fsEntryDescs;
};

typedef struct {
    Result (*translateBlockPos)(iNode* iNode, Index64* vBlockIndex, Size* n, Range* pBlockRanges, Size rangeN);

    Result (*resize)(iNode* iNode, Size newSizeInByte);
} iNodeOperations;

STRUCT_PRIVATE_DEFINE(iNode) {
    Uint32                  signature;
#define INODE_SIGNATURE     0x120DE516
    Size                    sizeInBlock;
    SuperBlock*             superBlock;
    Uint32                  openCnt;
    iNodeOperations*        operations;
    HashChainNode           openedNode;
    SinglyLinkedListNode    mountNode;

    union {
        Object              specificInfo;
        Device*             device; //Only for when iNode is mapped to a device
    };
};

typedef enum {
    FS_ENTRY_TYPE_DUMMY,
    FS_ENTRY_TYPE_FILE,
    FS_ENTRY_TYPE_DIRECTORY,
    FS_ENTRY_TYPE_DEVICE
} fsEntryType;

//Indicates the unique position of a fs entry
STRUCT_PRIVATE_DEFINE(fsEntryIdentifier) {
    String      name;
    String      parentPath;
    fsEntryType type;
};

typedef struct {
    Index64 (*seek)(fsEntry* entry, Index64 seekTo);

    Result  (*read)(fsEntry* entry, void* buffer, Size n);

    Result  (*write)(fsEntry* entry, const void* buffer, Size n);

    Result  (*resize)(fsEntry* entry, Size newSizeInByte);
} fsEntryOperations;

typedef struct {
    //Pointer points to current entry
    Result  (*readEntry)(Directory* directory, fsEntryDesc* childDesc, Size* entrySizePtr);

    Result  (*addEntry)(Directory* directory, fsEntryDesc* childToAdd);

    //Pointer points to entry to remove
    Result  (*removeEntry)(Directory* directory, fsEntryIdentifier* childToRemove);

    //Pointer points to entry to update
    Result  (*updateEntry)(Directory* directory, fsEntryIdentifier* oldChild, fsEntryDesc* newChild);
} fsEntryDirOperations;

//Gives full description of a fs entry
STRUCT_PRIVATE_DEFINE(fsEntryDesc) {
    fsEntryIdentifier   identifier; //One descriptor, one identifier

    union {
        Range           dataRange;  //Position and size on device (In byte)
        DeviceID        device;
    };
    
#define FS_ENTRY_INVALID_POSITION       -1
#define FS_ENTRY_INVALID_SIZE           -1
    Flags16             flags;
#define FS_ENTRY_DESC_FLAGS_READ_ONLY   FLAG8(0)
#define FS_ENTRY_DESC_FLAGS_HIDDEN      FLAG8(1)
#define FS_ENTRY_DESC_FLAGS_MOUNTED     FLAG8(2)

    Uint64              createTime;
    Uint64              lastAccessTime;
    Uint64              lastModifyTime;

    Uint32              descReferCnt;
    HashChainNode       descNode;
};

typedef struct {
    ConstCstring    name;
    ConstCstring    parentPath;
    fsEntryType     type;
    bool            isDevice;
    union {
        Range       dataRange;
        DeviceID    device;
    };
    Flags16         flags;
    Uint64          createTime;
    Uint64          lastAccessTime;
    Uint64          lastModifyTime;
} fsEntryDescInitArgs;

//Real fs entry for process
STRUCT_PRIVATE_DEFINE(fsEntry) {    //TODO: Add RW lock
    fsEntryDesc*            desc;   //Meaning multiple fs entries can share single descriptor
    Index64                 pointer;
    iNode*                  iNode;
    fsEntryOperations*      operations;
    fsEntryDirOperations*   dirOperations;
    SinglyLinkedList        mounted;
};

#endif // __FS_STRUCT_H
