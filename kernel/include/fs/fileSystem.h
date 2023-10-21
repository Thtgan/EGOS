#if !defined(__FILESYSTEM_H)
#define __FILESYSTEM_H

#include<fs/fileSystemEntry.h>
#include<fs/fsPreDefines.h>
#include<fs/inode.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef enum {
    FILE_SYSTEM_TYPE_PHOSPHERUS,
    FILE_SYSTEM_TYPE_FAT32,
    FILE_SYSTEM_TYPE_NUM,
    FILE_SYSTEM_TYPE_UNKNOWN
} FileSystemType;

typedef struct {
    Result  (*openInode)(SuperBlock* superBlock, iNode* iNode, FileSystemEntryDescriptor* entryDescripotor);
    Result  (*closeInode)(SuperBlock* superBlock, iNode* iNode);

    Result  (*openFileSystemEntry)(SuperBlock* superBlock, FileSystemEntry* entry, FileSystemEntryDescriptor* entryDescripotor);
    Result  (*closeFileSystemEntry)(SuperBlock* superBlock, FileSystemEntry* entry);
} SuperBlockOperations;

STRUCT_PRIVATE_DEFINE(SuperBlock) { //TODO: Try fix this with a file with pre-defines
    BlockDevice*                device;
    SuperBlockOperations*       operations;

    FileSystemEntry*            rootDirectory;
    void*                       specificInfo;
};

typedef struct {
    ConstCstring    name;
    FileSystemType  type;
    HashChainNode   managerNode;
    SuperBlock*     superBlock;
} FileSystem;

/**
 * @brief Initialize the file system
 * 
 * @return Result Result of the operation
 */
Result initFileSystem();

/**
 * @brief Deploy file system on device, sets errorcode to indicate error
 * 
 * @param device Device to mount the file system
 * @param type File system type to deploy
 * @return Result Result of the operation
 */
Result deployFileSystem(BlockDevice* device, FileSystemType type);

/**
 * @brief Check type of file system on device
 * 
 * @param device Device to check
 * @return FileSystemTypes Type of file system
 */
FileSystemType checkFileSystem(BlockDevice* device);

/**
 * @brief Open file system on device, return from buffer if file system is open, sets errorcode to indicate error
 * 
 * @param device Device to open
 * @return FileSystem* File system, NULL if error happens
 */
FileSystem* openFileSystem(BlockDevice* device);

/**
 * @brief Close a opened file system, cannot close a closed file system, sets errorcode to indicate error
 * 
 * @param system File system
 * @return Result Result of the operation, NULL if error happens
 */
Result closeFileSystem(FileSystem* fs);

static inline Result rawSuperNodeOpenInode(SuperBlock* superBlock, iNode* iNode, FileSystemEntryDescriptor* entryDescripotor) {
    return superBlock->operations->openInode(superBlock, iNode, entryDescripotor);
}

static inline Result rawSuperNodeCloseInode(SuperBlock* superBlock, iNode* iNode) {
    return superBlock->operations->closeInode(superBlock, iNode);
}

static inline Result rawSuperNodeOpenFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry, FileSystemEntryDescriptor* entryDescripotor) {
    return superBlock->operations->openFileSystemEntry(superBlock, entry, entryDescripotor);
}

static inline Result rawSuperNodeCloseFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry) {
    return superBlock->operations->closeFileSystemEntry(superBlock, entry);
}

#endif // __FILESYSTEM_H
