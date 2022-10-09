#if !defined(__FILESYSTEM_H)
#define __FILESYSTEM_H

#include<fs/directory.h>
#include<fs/file.h>
#include<fs/inode.h>
#include<kit/types.h>

typedef enum {
    FILE_SYSTEM_TYPE_PHOSPHERUS,
    FILE_SYSTEM_TYPE_UNKNOWN
} FileSystemType;

typedef struct {
    File* (*openFile)(iNode* iNode);
    int (*closeFile)(File* file);
} FileGlobalOperations;

typedef struct {
    Directory* (*openDirectory)(iNode* iNode);
    int (*closeDirectory)(Directory* directory);
} DirectoryGlobalOperations;

STRUCT_PRE_DEFINE(FileSystem);

typedef struct {
    /**
     * @brief Create a iNode with given size on device
     * 
     * @param type type of the iNode
     * @return Index64 The index of the block where the iNode is located
     */
    Index64 (*createInode)(THIS_ARG_APPEND(FileSystem, iNodeType type));

    /**
     * @brief Delete iNode from device
     * 
     * @param iNodeBlock Block index where the inode is located
     * @return int Status of the operation
     */
    int (*deleteInode)(THIS_ARG_APPEND(FileSystem, Index64 iNodeBlock));

    /**
     * @brief Open a inode through on the given block
     * 
     * @param iNodeBlock Block where the inode is located
     * @return iNode* Opened iNode
     */
    iNode* (*openInode)(THIS_ARG_APPEND(FileSystem, Index64 iNodeBlock));

    /**
     * @brief Close the inode opened
     * 
     * @param iNode iNode to close
     * @return int Status of the operation
     */
    int (*closeInode)(iNode* iNode);
} iNodeGlobalOperations;

typedef struct {
    iNodeGlobalOperations* iNodeGlobalOperations;
    FileGlobalOperations* fileGlobalOperations;
    DirectoryGlobalOperations* directoryGlobalOperations;
} FileSystemOperations;

STRUCT_PRIVATE_DEFINE(FileSystem) {
    char name[32];
    FileSystemType type;
    FileSystemOperations* opearations;
    ID device;
    Index64 rootDirectoryInode;
    void* data;
};

/**
 * @brief Initialize the file system
 * 
 * @param type File system type to initialize
 */
void initFileSystem(FileSystemType type);

/**
 * @brief Deploy file system on device
 * 
 * @param device Device to mount the file system
 * @param type File system type to deploy
 * @return Is deployment succeeded
 */
bool deployFileSystem(BlockDevice* device, FileSystemType type);

/**
 * @brief Check type of file system on device
 * 
 * @param device DEvice to check
 * @return FileSystemTypes Type of file system
 */
FileSystemType checkFileSystem(BlockDevice* device);

/**
 * @brief Open file system on device
 * 
 * @param device Device to open
 * @param type File system type
 * @return FileSystem* File system
 */
FileSystem* openFileSystem(BlockDevice* device, FileSystemType type);

/**
 * @brief Close a opened file system, cannot close a closed file system
 * 
 * @param system File system
 */
void closeFileSystem(FileSystem* system);

#endif // __FILESYSTEM_H
