#if !defined(__FILE_SYSTEM_ENTRY)
#define __FILE_SYSTEM_ENTRY

#include<fs/fileSystem.h>
#include<fs/fsPreDefines.h>
#include<fs/inode.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<string.h>

typedef enum {
    FILE_SYSTEM_ENTRY_TYPE_DUMMY,
    FILE_SYSTEM_ENTRY_TYPE_FILE,
    FILE_SYSTEM_ENTRY_TYPE_DIRECTOY
} FileSystemEntryType;

typedef struct {
    Index64 (*seek)(FileSystemEntry* entry, Index64 seekTo);

    Result (*read)(FileSystemEntry* entry, void* buffer, Size n);

    Result (*write)(FileSystemEntry* entry, const void* buffer, Size n);

    Result (*resize)(FileSystemEntry* entry, Size newSizeInByte);

    //Pointer points to current entry
    Result (*readChild)(FileSystemEntry* directory, FileSystemEntryDescriptor* childDesc, Size* entrySizePtr);

    Result (*addChild)(FileSystemEntry* directory, FileSystemEntryDescriptor* childToAdd);

    //Pointer points to entry to remove
    Result (*removeChild)(FileSystemEntry* directory, FileSystemEntryIdentifier* childToRemove);

    //Pointer points to entry to uipdate
    Result (*updateChild)(FileSystemEntry* directory, FileSystemEntryIdentifier* oldChild, FileSystemEntryDescriptor* newChild);
} FileSystemEntryOperations;

STRUCT_PRIVATE_DEFINE(FileSystemEntryIdentifier) {
    ConstCstring                name;
    FileSystemEntryType         type;
    FileSystemEntryDescriptor*  parent;
};

STRUCT_PRIVATE_DEFINE(FileSystemEntryDescriptor) {
    FileSystemEntryIdentifier   identifier;
    Range                       dataRange;  //In byte
#define FILE_SYSTEM_ENTRY_INVALID_POSITION  -1
#define FILE_SYSTEM_ENTRY_INVALID_SIZE      -1
    Flags16                     flags;
#define FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_PRESENT     FLAG8(0)
#define FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_READ_ONLY   FLAG8(1)
#define FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_HIDDEN      FLAG8(2)

    Uint64                      createTime;
    Uint64                      lastAccessTime;
    Uint64                      lastModifyTime;
};

STRUCT_PRIVATE_DEFINE(FileSystemEntry) {    //TODO: Add RW lock
    FileSystemEntryDescriptor*  descriptor;

    Index64                     pointer;
    iNode*                      iNode;
    FileSystemEntryOperations*  operations;
};

static inline Index64 rawFileSystemEntrySeek(FileSystemEntry* entry, Size seekTo) {
    return entry->operations->seek(entry, seekTo);
} 

static inline Result rawFileSystemEntryRead(FileSystemEntry* entry, void* buffer, Size n) {
    return entry->operations->read(entry, buffer, n);
} 

static inline Result rawFileSystemEntryWrite(FileSystemEntry* entry, const void* buffer, Size n) {
    return entry->operations->write(entry, buffer, n);
}

static inline Result rawFileSystemEntryResize(FileSystemEntry* entry, Size newBlockSize) {
    return entry->operations->resize(entry, newBlockSize);
}

static inline Result rawFileSystemEntryReadChild(FileSystemEntry* directory, FileSystemEntryDescriptor* childDesc, Size* entrySizePtr) {
    return directory->operations->readChild(directory, childDesc, entrySizePtr);
}

static inline Result rawFileSystemEntryAddChild(FileSystemEntry* directory, FileSystemEntryDescriptor* childToAdd) {
    return directory->operations->addChild(directory, childToAdd);
}

static inline Result rawFileSystemEntryRemoveChild(FileSystemEntry* directory, FileSystemEntryIdentifier* childToRemove) {
    return directory->operations->removeChild(directory, childToRemove);
}

static inline Result rawFileSystemEntryUpdateChild(FileSystemEntry* directory, FileSystemEntryIdentifier* oldChild, FileSystemEntryDescriptor* newChild) {
    return directory->operations->updateChild(directory, oldChild, newChild);
}

typedef struct {
    ConstCstring name;
    FileSystemEntryType type;
    Range dataRange;
    FileSystemEntryDescriptor* parent;
    Flags16 flags;
    Uint64                      createTime;
    Uint64                      lastAccessTime;
    Uint64                      lastModifyTime;
} FileSystemEntryDescriptorInitArgs;

Result initFileSystemEntryDescriptor(FileSystemEntryDescriptor* desc, FileSystemEntryDescriptorInitArgs* args);

void clearFileSystemEntryDescriptor(FileSystemEntryDescriptor* desc);

Result genericOpenFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry, FileSystemEntryDescriptor* entryDescriptor);

Result genericCloseFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry);

Index64 genericFileSystemEntrySeek(FileSystemEntry* entry, Index64 seekTo);

Result genericFileSystemEntryRead(FileSystemEntry* entry, void* buffer, Size n);

Result genericFileSystemEntryWrite(FileSystemEntry* entry, const void* buffer, Size n);

Result fileSystemEntryOpen(FileSystemEntry* entry, FileSystemEntryDescriptor* desc, ConstCstring path, FileSystemEntryType type);

Result fileSystemEntryClose(FileSystemEntry* entry);

#endif // __FILE_SYSTEM_ENTRY
