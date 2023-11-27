#if !defined(__FS_ENTRY)
#define __FS_ENTRY

#include<fs/fs.h>
#include<fs/fsPreDefines.h>
#include<fs/inode.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<string.h>

typedef enum {
    FS_ENTRY_TYPE_DUMMY,
    FS_ENTRY_TYPE_FILE,
    FS_ENTRY_TYPE_DIRECTORY
} FSentryType;

typedef struct {
    Index64 (*seek)(FSentry* entry, Index64 seekTo);

    Result  (*read)(FSentry* entry, void* buffer, Size n);

    Result  (*write)(FSentry* entry, const void* buffer, Size n);

    Result  (*resize)(FSentry* entry, Size newSizeInByte);

    //Pointer points to current entry
    Result  (*readChild)(Directory* directory, FSentryDesc* childDesc, Size* entrySizePtr);

    Result  (*addChild)(Directory* directory, FSentryDesc* childToAdd);

    //Pointer points to entry to remove
    Result  (*removeChild)(Directory* directory, FSentryIdentifier* childToRemove);

    //Pointer points to entry to uipdate
    Result  (*updateChild)(Directory* directory, FSentryIdentifier* oldChild, FSentryDesc* newChild);
} FSentryOperations;

STRUCT_PRIVATE_DEFINE(FSentryIdentifier) {
    ConstCstring    name;
    FSentryType     type;
    FSentryDesc*    parent;
};

STRUCT_PRIVATE_DEFINE(FSentryDesc) {
    FSentryIdentifier   identifier;
    Range               dataRange;  //In byte
#define FS_ENTRY_INVALID_POSITION       -1
#define FS_ENTRY_INVALID_SIZE           -1
    Flags16             flags;
#define FS_ENTRY_DESC_FLAGS_PRESENT     FLAG8(0)
#define FS_ENTRY_DESC_FLAGS_READ_ONLY   FLAG8(1)
#define FS_ENTRY_DESC_FLAGS_HIDDEN      FLAG8(2)

    Uint64              createTime;
    Uint64              lastAccessTime;
    Uint64              lastModifyTime;
};

STRUCT_PRIVATE_DEFINE(FSentry) {    //TODO: Add RW lock
    FSentryDesc*        desc;
    Index64             pointer;
    iNode*              iNode;
    FSentryOperations*  operations;
};

static inline Index64 fsEntry_rawSeek(FSentry* entry, Size seekTo) {
    return entry->operations->seek(entry, seekTo);
} 

static inline Result fsEntry_rawRead(FSentry* entry, void* buffer, Size n) {
    return entry->operations->read(entry, buffer, n);
} 

static inline Result fsEntry_rawWrite(FSentry* entry, const void* buffer, Size n) {
    return entry->operations->write(entry, buffer, n);
}

static inline Result fsEntry_rawResize(FSentry* entry, Size newBlockSize) {
    return entry->operations->resize(entry, newBlockSize);
}

static inline Result fsEntry_rawReadChild(FSentry* directory, FSentryDesc* childDesc, Size* entrySizePtr) {
    return directory->operations->readChild(directory, childDesc, entrySizePtr);
}

static inline Result fsEntry_rawAddChild(FSentry* directory, FSentryDesc* childToAdd) {
    return directory->operations->addChild(directory, childToAdd);
}

static inline Result fsEntry_rawRemoveChild(FSentry* directory, FSentryIdentifier* childToRemove) {
    return directory->operations->removeChild(directory, childToRemove);
}

static inline Result fsEntry_rawUpdateChild(FSentry* directory, FSentryIdentifier* oldChild, FSentryDesc* newChild) {
    return directory->operations->updateChild(directory, oldChild, newChild);
}

typedef struct {
    ConstCstring    name;
    FSentryType     type;
    Range           dataRange;
    FSentryDesc*    parent;
    Flags16         flags;
    Uint64          createTime;
    Uint64          lastAccessTime;
    Uint64          lastModifyTime;
} FSentryDescInitStructArgs;

Result FSentryDesc_initStruct(FSentryDesc* desc, FSentryDescInitStructArgs* args);

void FSentryDesc_clearStruct(FSentryDesc* desc);

Result fsEntry_genericOpen(SuperBlock* superBlock, FSentry* entry, FSentryDesc* desc);

Result fsEntry_genericClose(SuperBlock* superBlock, FSentry* entry);

Index64 fsEntry_genericSeek(FSentry* entry, Index64 seekTo);

Result fsEntry_genericRead(FSentry* entry, void* buffer, Size n);

Result fsEntry_genericWrite(FSentry* entry, const void* buffer, Size n);

#endif // __FS_ENTRY
