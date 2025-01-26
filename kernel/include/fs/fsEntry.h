#if !defined(__FS_FSENTRY_H)
#define __FS_FSENTRY_H

typedef enum {
    FS_ENTRY_TYPE_DUMMY,
    FS_ENTRY_TYPE_FILE,
    FS_ENTRY_TYPE_DIRECTORY,
    FS_ENTRY_TYPE_DEVICE
} fsEntryType;

typedef struct fsEntryIdentifier fsEntryIdentifier;
typedef struct fsEntryDirOperations fsEntryDirOperations;
typedef struct fsEntryDesc fsEntryDesc;
typedef struct fsEntry fsEntry;
typedef struct fsEntryDescInitArgs fsEntryDescInitArgs;
typedef struct fsEntryOperations fsEntryOperations;

typedef fsEntry Directory;
typedef fsEntry File;

#include<cstring.h>
#include<devices/device.h>
#include<fs/fcntl.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/string.h>
#include<structs/hashTable.h>

//Indicates the unique position of a fs entry
typedef struct fsEntryIdentifier {
    String      name;
    String      parentPath;
    bool        isDirectory;
} fsEntryIdentifier;

void fsEntryIdentifier_initStruct(fsEntryIdentifier* identifier, ConstCstring path, bool isDirectory);

void fsEntryIdentifier_initStructSep(fsEntryIdentifier* identifier, ConstCstring parentPath, ConstCstring name, bool isDirectory);

void fsEntryIdentifier_initStructSepN(fsEntryIdentifier* identifier, ConstCstring parentPath, Size n1, ConstCstring name, Size n2, bool isDirectory);

static inline bool fsEntryIdentifier_isActive(fsEntryIdentifier* identifier) {
    return STRING_IS_AVAILABLE(&identifier->name) && STRING_IS_AVAILABLE(&identifier->parentPath);
}

void fsEntryIdentifier_clearStruct(fsEntryIdentifier* identifier);

void fsEntryIdentifier_copy(fsEntryIdentifier* des, fsEntryIdentifier* src);

void fsEntryIdentifier_getParent(fsEntryIdentifier* identifier, fsEntryIdentifier* parentIdentifierOut);

typedef struct fsEntryDirOperations {
    //Pointer points to current entry
    bool  (*readEntry)(Directory* directory, fsEntryDesc* childDesc, Size* entrySizePtr);

    void  (*addEntry)(Directory* directory, fsEntryDesc* childToAdd);

    //Pointer points to entry to remove
    void  (*removeEntry)(Directory* directory, fsEntryIdentifier* childToRemove);

    //Pointer points to entry to update
    void  (*updateEntry)(Directory* directory, fsEntryIdentifier* oldChild, fsEntryDesc* newChild);
} fsEntryDirOperations;

//Gives full description of a fs entry
typedef struct fsEntryDesc {
    fsEntryIdentifier   identifier; //One descriptor, one identifier
    fsEntryType         type;
    Uint16              mode;

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
} fsEntryDesc;

typedef struct fsEntryDescInitArgs {
    ConstCstring    name;
    ConstCstring    parentPath;
    fsEntryType     type;
    union {
        Range       dataRange;
        DeviceID    device;
    };
    Flags16         flags;
    Uint64          createTime;
    Uint64          lastAccessTime;
    Uint64          lastModifyTime;
} fsEntryDescInitArgs;

void fsEntryDesc_initStruct(fsEntryDesc* desc, fsEntryDescInitArgs* args);

void fsEntryDesc_clearStruct(fsEntryDesc* desc);

//Real fs entry for process
typedef struct fsEntry {    //TODO: Add RW lock
    fsEntryDesc*            desc;   //Meaning multiple fs entries can share single descriptor
    FCNTLopenFlags          openFlags;  //These flags control the IO detail of the entry
    Index64                 pointer;
    iNode*                  iNode;
    fsEntryOperations*      operations;
    fsEntryDirOperations*   dirOperations;
} fsEntry;

typedef struct fsEntryOperations {
    Index64 (*seek)(fsEntry* entry, Index64 seekTo);

    void  (*read)(fsEntry* entry, void* buffer, Size n);

    void  (*write)(fsEntry* entry, const void* buffer, Size n);

    void  (*resize)(fsEntry* entry, Size newSizeInByte);
} fsEntryOperations;

static inline Index64 fsEntry_rawSeek(fsEntry* entry, Size seekTo) {
    entry->operations->seek(entry, seekTo);
} 

static inline void fsEntry_rawRead(fsEntry* entry, void* buffer, Size n) {
    entry->operations->read(entry, buffer, n);
} 

static inline void fsEntry_rawWrite(fsEntry* entry, const void* buffer, Size n) {
    entry->operations->write(entry, buffer, n);
}

static inline void fsEntry_rawResize(fsEntry* entry, Size newSizeInByte) {
    entry->operations->resize(entry, newSizeInByte);
}

static inline bool fsEntry_rawDirReadEntry(fsEntry* directory, fsEntryDesc* entryDesc, Size* entrySizePtr) {
    return directory->dirOperations->readEntry(directory, entryDesc, entrySizePtr);
}

static inline void fsEntry_rawDirAddEntry(fsEntry* directory, fsEntryDesc* entryToAdd) {
    directory->dirOperations->addEntry(directory, entryToAdd);
}

static inline void fsEntry_rawDirRemoveEntry(fsEntry* directory, fsEntryIdentifier* entryToRemove) {
    directory->dirOperations->removeEntry(directory, entryToRemove);
}

static inline void fsEntry_rawDirUpdateEntry(fsEntry* directory, fsEntryIdentifier* oldEntry, fsEntryDesc* newEntry) {
    directory->dirOperations->updateEntry(directory, oldEntry, newEntry);
}

void fsEntry_genericOpen(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags);

void fsEntry_genericClose(SuperBlock* superBlock, fsEntry* entry);

Index64 fsEntry_genericSeek(fsEntry* entry, Index64 seekTo);

void fsEntry_genericRead(fsEntry* entry, void* buffer, Size n);

void fsEntry_genericWrite(fsEntry* entry, const void* buffer, Size n);

void fsEntry_genericResize(fsEntry* entry, Size newSizeInByte);

#endif // __FS_FSENTRY_H
