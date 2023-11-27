#if !defined(__FS_H)
#define __FS_H

#include<fs/fsEntry.h>
#include<fs/fsPreDefines.h>
#include<fs/inode.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef enum {
    FS_TYPE_FAT32,
    FS_TYPE_NUM,
    FS_TYPE_UNKNOWN
} FStype;

typedef struct {
    Result  (*openInode)(SuperBlock* superBlock, iNode* iNodePtr, FSentryDesc* desc);
    Result  (*closeInode)(SuperBlock* superBlock, iNode* iNode);

    Result  (*openFSentry)(SuperBlock* superBlock, FSentry* entry, FSentryDesc* desc);
    Result  (*closeFSentry)(SuperBlock* superBlock, FSentry* entry);
} SuperBlockOperations;

STRUCT_PRIVATE_DEFINE(SuperBlock) { //TODO: Try fix this with a file with pre-defines
    BlockDevice*                device;
    SuperBlockOperations*       operations;

    FSentry*                    rootDirectory;
    void*                       specificInfo;
    HashTable                   openedInode;
};

typedef struct {
    SuperBlock*     superBlock;
    ConstCstring    name;
    FStype          type;
} FS;

/**
 * @brief Initialize the file system
 * 
 * @return Result Result of the operation
 */
Result fs_init();

/**
 * @brief Check type of file system on device
 * 
 * @param device Device to check
 * @return FileSystemTypes Type of file system
 */
FStype fs_checkType(BlockDevice* device);

/**
 * @brief Open file system on device, return from buffer if file system is open, sets errorcode to indicate error
 * 
 * @param device Device to open
 * @return FS* File system, NULL if error happens
 */
Result fs_open(FS* fs, BlockDevice* device);

/**
 * @brief Close a opened file system, cannot close a closed file system, sets errorcode to indicate error
 * 
 * @param system File system
 * @return Result Result of the operation, NULL if error happens
 */
Result fs_close(FS* fs);

static inline Result superBlock_rawOpenInode(SuperBlock* superBlock, iNode* iNode, FSentryDesc* desc) {
    return superBlock->operations->openInode(superBlock, iNode, desc);
}

static inline Result superBlock_rawCloseInode(SuperBlock* superBlock, iNode* iNode) {
    return superBlock->operations->closeInode(superBlock, iNode);
}

static inline Result superBlock_rawOpenFSentry(SuperBlock* superBlock, FSentry* entry, FSentryDesc* desc) {
    return superBlock->operations->openFSentry(superBlock, entry, desc);
}

static inline Result superBlock_rawCloseFSentry(SuperBlock* superBlock, FSentry* entry) {
    return superBlock->operations->closeFSentry(superBlock, entry);
}

#endif // __FS_H
