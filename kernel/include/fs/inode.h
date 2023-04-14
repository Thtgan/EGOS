#if !defined(__INODE_H)
#define __INODE_H

#include<devices/block/blockDevice.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef enum {
    INODE_TYPE_UNKNOWN,
    INODE_TYPE_DIRECTORY,
    INODE_TYPE_FILE
} iNodeType;

//iNode ID: Device ID(16 bits) - iNode block index(48 bits)
#define BUILD_INODE_ID(__DEVICE_ID, __INODE_INDEX)  VAL_OR(VAL_LEFT_SHIFT((ID)(__DEVICE_ID), 48), (ID)__INODE_INDEX)
#define INODE_ID_GET_DEVICE_ID(__INODE_ID)          EXTRACT_VAL(__INODE_ID, 64, 48, 64)
#define INODE_ID_GET_INODE_INDEX(__INODE_ID)        EXTRACT_VAL(__INODE_ID, 64, 0, 48)

STRUCT_PRE_DEFINE(iNodeOperations);

typedef struct {
    struct __RecordOnDevice {
        uint64_t signature;
        size_t dataSize;            //iNode size in byte
        size_t availableBlockSize;  //How many blocks available for r/w?
        size_t blockTaken;          //How many block this iNode takes (including blocks used to maintain data)
        iNodeType type;
        size_t linkCnt;
        uint8_t data[464];          //Undefined data for different file systems
    } onDevice;

    BlockDevice* device;    //Which device is this iNode on
    Index64 blockIndex;     //Position on device
    uint32_t openCnt;       //How mauch is this iNode opened
    iNodeOperations* operations;
    void* entryReference;
    uint32_t referenceCnt;
    HashChainNode hashChainNode;
} iNode;

typedef struct __RecordOnDevice RecordOnDevice;

STRUCT_PRIVATE_DEFINE(iNodeOperations) {
    /**
     * @brief Resize the inode, will truncate the data if the removed part contains the data
     * 
     * @param iNode iNode
     * @param newBlockSize New size to resize to
     * @return int Status of the operation
     */
    int (*resize)(iNode* iNode, size_t newBlockSize);

    /**
     * @brief Read the data inside the iNode
     * 
     * @param iNode iNode
     * @param buffer Buffer to write to
     * @param blockIndexInNode First block to read, start from the beginning of the iNode
     * @param blockSize How many block(s) to read
     * @return int Status of the operation
     */
    int (*readBlocks)(iNode* iNode, void* buffer, size_t blockIndexInNode, size_t blockSize);

    /**
     * @brief Write the data inside the iNode
     * 
     * @param iNode iNode
     * @param buffer Buffer to read from
     * @param blockIndexInNode First block to write, start from the beginning of the iNode
     * @param blockSize How many block(s) to write
     * @return int Status of the operation
     */
    int (*writeBlocks)(iNode* iNode, const void* buffer, size_t blockIndexInNode, size_t blockSize);
};

/**
 * @brief Packed function of iNode operation
 * 
 * @param iNode iNode
 * @param newBlockSize New block size to resize to 
 * @return int 0 if succeeded
 */
static inline int iNodeResize(iNode* iNode, size_t newBlockSize) {
    return iNode->operations->resize(iNode, newBlockSize);
}

/**
 * @brief Packed function of iNode operation
 * 
 * @param iNode iNode
 * @param buffer Buffer to write to
 * @param blockIndexInNode Index of first block to read from
 * @param blockSize Num of block(s) to read
 * @return int 0 if succeeded
 */
static inline int iNodeReadBlocks(iNode* iNode, void* buffer, size_t blockIndexInNode, size_t blockSize) {
    return iNode->operations->readBlocks(iNode, buffer, blockIndexInNode, blockSize);
}

/**
 * @brief Packed function of iNode operation
 * 
 * @param iNode iNode
 * @param buffer Buffer to read from
 * @param blockIndexInNode Index of first block to write to
 * @param blockSize Num of block(s) to write
 * @return int 0 if succeeded
 */
static inline int iNodeWriteBlocks(iNode* iNode, const void* buffer, size_t blockIndexInNode, size_t blockSize) {
    return iNode->operations->writeBlocks(iNode, buffer, blockIndexInNode, blockSize);
}

#endif // __INODE_H
