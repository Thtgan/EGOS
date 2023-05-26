#if !defined(__INODE_H)
#define __INODE_H

#include<devices/block/blockDevice.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef enum {
    INODE_TYPE_UNKNOWN,
    INODE_TYPE_DIRECTORY,
    INODE_TYPE_FILE,
    INODE_TYPE_DEVICE
} iNodeType;

//iNode ID: Device ID(16 bits) - iNode block index(48 bits)
#define BUILD_INODE_ID(__DEVICE_ID, __INODE_INDEX)  VAL_OR(VAL_LEFT_SHIFT((ID)(__DEVICE_ID), 48), (ID)__INODE_INDEX)
#define INODE_ID_GET_DEVICE_ID(__INODE_ID)          EXTRACT_VAL(__INODE_ID, 64, 48, 64)
#define INODE_ID_GET_INODE_INDEX(__INODE_ID)        EXTRACT_VAL(__INODE_ID, 64, 0, 48)

STRUCT_PRE_DEFINE(iNodeOperations);

typedef struct {
    struct __RecordOnDevice {
        Uint64 signature;
        Size dataSize;            //iNode size in byte
        Size availableBlockSize;  //How many blocks available for r/w?
        Size blockTaken;          //How many block this iNode takes (including blocks used to maintain data)
        iNodeType type;
        Size linkCnt;
        Uint8 data[464];          //Undefined data for different file systems
    } onDevice;

    BlockDevice* device;    //Which device is this iNode on
    Index64 blockIndex;     //Position on device
    Uint32 openCnt;         //How mauch is this iNode opened
    iNodeOperations* operations;
    HashChainNode hashChainNode;
} iNode;

typedef struct __RecordOnDevice RecordOnDevice;

STRUCT_PRIVATE_DEFINE(iNodeOperations) {
    /**
     * @brief Resize the inode, will truncate the data if the removed part contains the data, sets errorcode to indicate error
     * 
     * @param iNode iNode
     * @param newBlockSize New size to resize to
     * @return Result Result of the operation
     */
    Result (*resize)(iNode* iNode, Size newBlockSize);

    /**
     * @brief Read the data inside the iNode, sets errorcode to indicate error
     * 
     * @param iNode iNode
     * @param buffer Buffer to write to
     * @param blockIndexInNode First block to read, start from the beginning of the iNode
     * @param blockSize How many block(s) to read
     * @return Result Result of the operation
     */
    Result (*readBlocks)(iNode* iNode, void* buffer, Size blockIndexInNode, Size blockSize);

    /**
     * @brief Write the data inside the iNode, sets errorcode to indicate error
     * 
     * @param iNode iNode
     * @param buffer Buffer to read from
     * @param blockIndexInNode First block to write, start from the beginning of the iNode
     * @param blockSize How many block(s) to write
     * @return Result Result of the operation
     */
    Result (*writeBlocks)(iNode* iNode, const void* buffer, Size blockIndexInNode, Size blockSize);
};

/**
 * @brief Packed function of iNode operation, sets errorcode to indicate error
 * 
 * @param iNode iNode
 * @param newBlockSize New block size to resize to 
 * @return Result Result of the operation
 */
static inline Result rawInodeResize(iNode* iNode, Size newBlockSize) {
    return iNode->operations->resize(iNode, newBlockSize);
}

/**
 * @brief Packed function of iNode operation, sets errorcode to indicate error
 * 
 * @param iNode iNode
 * @param buffer Buffer to write to
 * @param blockIndexInNode Index of first block to read from
 * @param blockSize Num of block(s) to read
 * @return Result Result of the operation
 */
static inline Result rawInodeReadBlocks(iNode* iNode, void* buffer, Size blockIndexInNode, Size blockSize) {
    return iNode->operations->readBlocks(iNode, buffer, blockIndexInNode, blockSize);
}

/**
 * @brief Packed function of iNode operation, sets errorcode to indicate error
 * 
 * @param iNode iNode
 * @param buffer Buffer to read from
 * @param blockIndexInNode Index of first block to write to
 * @param blockSize Num of block(s) to write
 * @return Result Result of the operation
 */
static inline Result rawInodeWriteBlocks(iNode* iNode, const void* buffer, Size blockIndexInNode, Size blockSize) {
    return iNode->operations->writeBlocks(iNode, buffer, blockIndexInNode, blockSize);
}

#endif // __INODE_H
