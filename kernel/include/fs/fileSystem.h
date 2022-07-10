#if !defined(__FILESYSTEM_H)
#define __FILESYSTEM_H

#include<fs/blockDevice/blockDevice.h>
#include<kit/oop.h>
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
    block_index_t   (*createFile)   (THIS_ARG_APPEND_NO_ARG(FileSystem));
    bool            (*deleteFile)   (THIS_ARG_APPEND(FileSystem, block_index_t inode));
    FilePtr         (*openFile)     (THIS_ARG_APPEND(FileSystem, block_index_t inode));
    void            (*closeFile)    (THIS_ARG_APPEND(FileSystem, FilePtr file));
    size_t          (*getFileSize)  (THIS_ARG_APPEND(FileSystem, FilePtr file));
    void            (*seekFile)     (THIS_ARG_APPEND(FileSystem, FilePtr file, size_t pointer));
    size_t          (*readFile)     (THIS_ARG_APPEND(FileSystem, FilePtr file, void* buffer, size_t n));
    size_t          (*writeFile)    (THIS_ARG_APPEND(FileSystem, FilePtr file, const void* buffer, size_t n));
    bool            (*truncateFile) (THIS_ARG_APPEND(FileSystem, FilePtr file, size_t truncateAt));
} FileOperations;

typedef struct {
    DirectoryPtr    (*getRootDirectory)                 (THIS_ARG_APPEND_NO_ARG(FileSystem));
    block_index_t   (*createDirectory)                  (THIS_ARG_APPEND_NO_ARG(FileSystem));
    bool            (*deleteDirectory)                  (THIS_ARG_APPEND(FileSystem, block_index_t inode));
    DirectoryPtr    (*openDirectory)                    (THIS_ARG_APPEND(FileSystem, block_index_t inode));
    void            (*closeDirectory)                   (THIS_ARG_APPEND(FileSystem, DirectoryPtr directory));
    size_t          (*getDirectoryItemNum)              (THIS_ARG_APPEND(FileSystem, DirectoryPtr directory));
    size_t          (*searchDirectoryItem)              (THIS_ARG_APPEND(FileSystem, DirectoryPtr directory, const char* itemName, bool isDirectory));
    block_index_t   (*getDirectoryItemInode)            (THIS_ARG_APPEND(FileSystem, DirectoryPtr directory, size_t index));
    bool            (*checkIsDirectoryItemDirectory)    (THIS_ARG_APPEND(FileSystem, DirectoryPtr directory, size_t index));
    void            (*readDirectoryItemName)            (THIS_ARG_APPEND(FileSystem, DirectoryPtr directory, size_t index, char* buffer, size_t n));
    block_index_t   (*removeDirectoryItem)              (THIS_ARG_APPEND(FileSystem, DirectoryPtr directory, size_t itemIndex));
    bool            (*insertDirectoryItem)              (THIS_ARG_APPEND(FileSystem, DirectoryPtr directory, block_index_t inode, const char* itemName, bool isDirectory));
} PathOperations;

struct __FileSystem {
    char name[32];
    FileSystemTypes type;
    void* specificFileSystem;
    FileOperations fileOperations;
    PathOperations pathOperations;
};

/**
 * @brief Initialize the file system
 * 
 * @param type File system type to initialize
 */
void initFileSystem(FileSystemTypes type);

/**
 * @brief Deploy file system on device
 * 
 * @param device Device to mount the file system
 * @param type File system type to deploy
 * @return Is deployment succeeded
 */
bool deployFileSystem(BlockDevice* device, FileSystemTypes type);

/**
 * @brief Check type of file system on device
 * 
 * @param device DEvice to check
 * @return FileSystemTypes Type of file system
 */
FileSystemTypes checkFileSystem(BlockDevice* device);

/**
 * @brief Open file system on device
 * 
 * @param device Device to open
 * @param type File system type
 * @return FileSystem* File system
 */
FileSystem* openFileSystem(BlockDevice* device, FileSystemTypes type);

/**
 * @brief Close a opened file system, cannot close a closed file system
 * 
 * @param system File system
 */
void closeFileSystem(FileSystem* system);

#endif // __FILESYSTEM_H
