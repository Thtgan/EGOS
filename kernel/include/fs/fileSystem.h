#if !defined(__FILESYSTEM_H)
#define __FILESYSTEM_H

#include<fs/directory.h>
#include<fs/file.h>
#include<fs/inode.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef enum {
    FILE_SYSTEM_TYPE_PHOSPHERUS,
    FILE_SYSTEM_TYPE_NUM,
    FILE_SYSTEM_TYPE_UNKNOWN
} FileSystemType;

typedef struct {
    /**
     * @brief Open the file in iNode
     * 
     * @param iNode iNode contains the file
     * @return File* File opened
     */
    File* (*openFile)(iNode* iNode);

    /**
     * @brief Close the file opened, be awared that iNode is not closed
     * 
     * @param file File to close
     * @return int 0 if succeeded
     */
    int (*closeFile)(File* file);
} FileGlobalOperations;

typedef struct {
    /**
     * @brief Open the directory in iNode
     * 
     * @param iNode iNode contains the directory
     * @return Directory* Directory opened
     */
    Directory* (*openDirectory)(iNode* iNode);

    /**
     * @brief Close the directory opened, be awared that iNode is not closed
     * 
     * @param directory Directory to close
     * @return int 0 if succeeded
     */
    int (*closeDirectory)(Directory* directory);
} DirectoryGlobalOperations;

STRUCT_PRE_DEFINE(FileSystem);

typedef struct {
    /**
     * @brief Create a iNode with given size on this file system
     * 
     * @param type type of the iNode
     * @return Index64 The index of the block where the iNode is located
     */
    Index64 (*createInode)(FileSystem* fs, iNodeType type);

    /**
     * @brief Delete iNode from this file system
     * 
     * @param iNodeBlock Block index where the inode is located
     * @return int Status of the operation
     */
    int (*deleteInode)(FileSystem* fs, Index64 iNodeBlock);

    /**
     * @brief Open a inode through on the given block
     * 
     * @param iNodeBlock Block where the inode is located
     * @return iNode* Opened iNode
     */
    iNode* (*openInode)(FileSystem* fs, Index64 iNodeBlock);

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
    HashChainNode managerNode;
    ID device;
    Index64 rootDirectoryInodeIndex;
    void* data;
};

/**
 * @brief Initialize the file system
 */
void initFileSystem();

/**
 * @brief Deploy file system on device
 * 
 * @param device Device to mount the file system
 * @param type File system type to deploy
 * @return int 0 if succeeded
 */
int deployFileSystem(BlockDevice* device, FileSystemType type);

/**
 * @brief Check type of file system on device
 * 
 * @param device Device to check
 * @return FileSystemTypes Type of file system
 */
FileSystemType checkFileSystem(BlockDevice* device);

/**
 * @brief Open file system on device, return from buffer if file system is open
 * 
 * @param device Device to open
 * @return FileSystem* File system
 */
FileSystem* openFileSystem(BlockDevice* device);

/**
 * @brief Close a opened file system, cannot close a closed file system
 * 
 * @param system File system
 * @return int 0 if succeeded
 */
int closeFileSystem(FileSystem* fs);

#endif // __FILESYSTEM_H
