#if !defined(__FILESYSTEM_H)
#define __FILESYSTEM_H

#include<fs/volume.h>
#include<kit/oop.h>
#include<kit/types.h>

typedef enum {
    FILE_SYSTEM_TYPE_FAT32,
    FILE_SYSTEM_TYPE_NUM,
    FILE_SYSTEM_TYPE_UNKNOWN
} FileSystemType;

typedef enum {
    FILE_SYSTEM_ENTRY_TYPE_DUMMY,
    FILE_SYSTEM_ENTRY_TYPE_FILE,
    FILE_SYSTEM_ENTRY_TYPE_DIRECTOY
} FileSystemEntryType;

STRUCT_PRE_DEFINE(FileSystemEntry);

typedef struct {
    OldResult  (*fileSystemEntryOpen)(Volume* v, ConstCstring path, FileSystemEntry* entry, FileSystemEntryType type);

    void    (*fileSystemEntryClose)(FileSystemEntry* entry);
} FileSystemOperations;

typedef struct {
    ConstCstring            name;
    FileSystemType          type;
    FileSystemOperations*   fileSystemOperations;
    void*                   context;
} FileSystem;

FileSystemType checkFileSystem(Volume* v);

OldResult openFileSystem(Volume* v);

typedef struct {
    Index32 (*seek)(FileSystemEntry* file, Index32 seekTo);

    OldResult  (*read)(FileSystemEntry* file, void* buffer, Size n);
} FileOperations;

typedef struct {
    OldResult (*lookupEntry)(FileSystemEntry* directory, ConstCstring name, FileSystemEntryType type, FileSystemEntry* entry, Index32* entryIndex);
} DirectoryOperations;

#define FILE_SYSTEM_ENTRY_FLAGS_READ_ONLY       FLAG8(0)
#define FILE_SYSTEM_ENTRY_FLAGS_HIDDEN          FLAG8(1)

STRUCT_PRIVATE_DEFINE(FileSystemEntry) {
    ConstCstring                name;
    Size                        nameBufferSize;

    Volume*                     volume;

    Flags8                      flags;
    FileSystemEntryType         type;

    union {
        FileOperations*         fileOperations;
        DirectoryOperations*    directoryOperations;
    };

    void*                       context;
    void*                       handle;
};

static inline Index32 rawFileSeek(FileSystemEntry* file, Size seekTo) {
    if (file->type != FILE_SYSTEM_ENTRY_TYPE_FILE || file->fileOperations->seek == NULL) {
        return INVALID_INDEX;
    }
    return file->fileOperations->seek(file, seekTo);
} 

static inline OldResult rawFileRead(FileSystemEntry* file, void* buffer, Size n) {
    if (file->type != FILE_SYSTEM_ENTRY_TYPE_FILE || file->fileOperations->read == NULL) {
        return RESULT_ERROR;
    }
    return file->fileOperations->read(file, buffer, n);
}

static inline Index32 rawDirectoryLookupEntry(FileSystemEntry* directory, ConstCstring name, FileSystemEntryType type, FileSystemEntry* entry, Index32* entryIndex) {
    if (directory->type != FILE_SYSTEM_ENTRY_TYPE_DIRECTOY || directory->directoryOperations->lookupEntry == NULL) {
        return INVALID_INDEX;
    }
    return directory->directoryOperations->lookupEntry(directory, name, type, entry, entryIndex);
}

static inline OldResult rawFileSystemEntryOpen(Volume* v, ConstCstring path, FileSystemEntry* entry, FileSystemEntryType type) {
    if (v == NULL || v->fileSystem == NULL || path == NULL || entry == NULL || type == FILE_SYSTEM_ENTRY_TYPE_DUMMY) {
        return RESULT_ERROR; 
    }
    return ((FileSystem*)v->fileSystem)->fileSystemOperations->fileSystemEntryOpen(v, path, entry, type);
}

static inline OldResult rawFileSystemEntryClose(FileSystemEntry* entry) {
    if (entry == NULL || entry->volume == NULL || entry->volume->fileSystem == NULL) {
        return RESULT_ERROR; 
    }
    ((FileSystem*)entry->volume->fileSystem)->fileSystemOperations->fileSystemEntryClose(entry);
}

#endif // __FILESYSTEM_H
