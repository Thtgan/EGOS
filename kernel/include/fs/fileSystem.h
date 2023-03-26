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
    Index64 (*createInode)(FileSystem* fs, iNodeType type);

    /**
     * @brief Delete iNode from device
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
    Index64 rootDirectoryInode;
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
 * @return FileSystem* File system
 */
FileSystem* openFileSystem(BlockDevice* device);

/**
 * @brief Close a opened file system, cannot close a closed file system
 * 
 * @param system File system
 */
bool closeFileSystem(BlockDevice* device);

File* openFile(iNode* iNode);

int closeFile(File* file);

Directory* openDirectory(iNode* iNode);

int closeDirectory(Directory* directory);

Index64 createInode(iNodeType type);

int deleteInode(Index64 iNodeBlock);

iNode* openInode(Index64 iNodeBlock);

int closeInode(iNode* iNode);

#endif // __FILESYSTEM_H
