#if !defined(__FS_FSENTRY_H)
#define __FS_FSENTRY_H

typedef enum {
    FS_ENTRY_TYPE_DUMMY,
    FS_ENTRY_TYPE_FILE,
    FS_ENTRY_TYPE_DIRECTORY,
    FS_ENTRY_TYPE_DEVICE
} fsEntryType;

typedef struct fsEntry fsEntry;
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
#include<structs/refCounter.h>

//Real fs entry for process
typedef struct fsEntry {    //TODO: Add RW lock
    FCNTLopenFlags          flags;  //These flags control the IO detail of the entry
    Uint16                  mode;
    Index64                 pointer;
    iNode*                  inode;
    fsEntryOperations*      operations;
} fsEntry;

typedef struct fsEntryOperations {
    Index64 (*seek)(fsEntry* entry, Index64 seekTo);

    void (*read)(fsEntry* entry, void* buffer, Size n);

    void (*write)(fsEntry* entry, const void* buffer, Size n);
} fsEntryOperations;

static inline bool fsEntryType_isDevice(fsEntryType type) {
    return type == FS_ENTRY_TYPE_DEVICE;
}

static inline Index64 fsEntry_rawSeek(fsEntry* entry, Size seekTo) {
    return entry->operations->seek(entry, seekTo);
} 

static inline void fsEntry_rawRead(fsEntry* entry, void* buffer, Size n) {
    entry->operations->read(entry, buffer, n);
} 

static inline void fsEntry_rawWrite(fsEntry* entry, const void* buffer, Size n) {
    entry->operations->write(entry, buffer, n);
}

void fsEntry_initStruct(fsEntry* entry, iNode* inode, fsEntryOperations* operations, FCNTLopenFlags flags);

void fsEntry_clearStruct(fsEntry* entry);

Index64 fsEntry_genericSeek(fsEntry* entry, Index64 seekTo);

void fsEntry_genericRead(fsEntry* entry, void* buffer, Size n);

void fsEntry_genericWrite(fsEntry* entry, const void* buffer, Size n);

#endif // __FS_FSENTRY_H
