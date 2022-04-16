#if !defined(__FILESYSTEM_H)
#define __FILESYSTEM_H

#include<fs/blockDevice/blockDevice.h>
#include<stddef.h>

typedef enum {
    FILE_SYSTEM_TYPE_PHOSPHERUS,
    FILE_SYSTEM_TYPE_NULL
} FileSystemTypes;

typedef void* FilePtr;

typedef void* DirectoryPtr;

struct __FileSystem;

typedef struct __FileSystem FileSystem;

typedef struct {
    block_index_t   (*createFile)   (FileSystem* fileSystem);
    bool            (*deleteFile)   (FileSystem* fileSystem, block_index_t inode);
    FilePtr         (*openFile)     (FileSystem* fileSystem, block_index_t inode);
    void            (*closeFile)    (FileSystem* fileSystem, FilePtr file);
    size_t          (*getFileSize)  (FileSystem* fileSystem, FilePtr file);
    void            (*seekFile)     (FileSystem* fileSystem, FilePtr file, size_t pointer);
    size_t          (*readFile)     (FileSystem* fileSystem, FilePtr file, void* buffer, size_t n);
    size_t          (*writeFile)    (FileSystem* fileSystem, FilePtr file, const void* buffer, size_t n);
    bool            (*truncateFile) (FileSystem* fileSystem, FilePtr file, size_t truncateAt);
} FileOperations;

typedef struct {
    DirectoryPtr    (*getRootDirectory)                 (FileSystem* fileSystem);
    block_index_t   (*createDirectory)                  (FileSystem* fileSystem);
    bool            (*deleteDirectory)                  (FileSystem* fileSystem, block_index_t inode);
    DirectoryPtr    (*openDirectory)                    (FileSystem* fileSystem, block_index_t inode);
    void            (*closeDirectory)                   (FileSystem* fileSystem, DirectoryPtr directory);
    size_t          (*getDirectoryItemNum)              (FileSystem* fileSystem, DirectoryPtr directory);
    size_t          (*searchDirectoryItem)              (FileSystem* fileSystem, DirectoryPtr directory, const char* itemName, bool isDirectory);
    block_index_t   (*getDirectoryItemInode)            (FileSystem* fileSystem, DirectoryPtr directory, size_t index);
    bool            (*checkIsDirectoryItemDirectory)    (FileSystem* fileSystem, DirectoryPtr directory, size_t index);
    void            (*readDirectoryItemName)            (FileSystem* fileSystem, DirectoryPtr directory, size_t index, char* buffer, size_t n);
    block_index_t   (*removeDirectoryItem)              (FileSystem* fileSystem, DirectoryPtr directory, size_t itemIndex);
    bool            (*insertDirectoryItem)              (FileSystem* fileSystem, DirectoryPtr directory, block_index_t inode, const char* itemName, bool isDirectory);
} PathOperations;

struct __FileSystem {
    char name[32];
    FileSystemTypes type;
    void* specificFileSystem;
    FileOperations fileOperations;
    PathOperations pathOperations;
};

void initFileSystem(FileSystemTypes type);

bool deployFileSystem(BlockDevice* device, FileSystemTypes type);

FileSystemTypes checkFileSystem(BlockDevice* device);

FileSystem* openFileSystem(BlockDevice* device, FileSystemTypes type);

void closeFileSystem(FileSystem* system);

#endif // __FILESYSTEM_H
