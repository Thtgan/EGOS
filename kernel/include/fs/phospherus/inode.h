#if !defined(__PHOSPHERUS_INODE_H)
#define __PHOSPHERUS_INODE_H

#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/allocator.h>
#include<structs/linkedList.h>

typedef struct {
    //TODO: Replace device name to more reliable device identification
    char deviceName[16];        //Name of the device
    block_index_t blockIndex;   //Index of the block this inode located
    uint32_t signature;         //Used to valid the iNode, maybe can be used for encryption
    uint32_t reserved1;         //Padding
    size_t blockSize;           //Num of the blocks contained by the inode
    size_t blockTaken;          //Num of blocks taken by inode
    size_t dataSize;            //Num of the bytes contained, designed to be set manually
    struct {
        block_index_t level0BlockTable[8];  //512B/Table
        block_index_t level1BlockTable[4];  //32KB/Table
        block_index_t level2BlockTable[2];  //2MB/Table
        block_index_t level3BlockTable;     //128MB/Table
        block_index_t level4BlockTable;     //8GB/Table
        block_index_t level5BlockTable;     //512GB/Table
    } __attribute__((packed)) blockTables;  //Similiar to unix, use recursive tables to record the positions of all blocks
    uint8_t reserved2[320];
} __attribute__((packed)) iNode;

typedef struct {
    LinkedListNode node;
    iNode* inode;
    uint16_t openCnt;
    Phospherus_Allocator* allocator;
    bool writeProtection;
} iNodeDesc;

/**
 * @brief Initialize the inode
 */
void phospherus_initInode();

/**
 * @brief Create a inode with given size
 * 
 * @param allocator Allocator
 * @param blockSize Size of the inode
 * @return block_index_t The index of the block where the inode is located, NULL if create failed
 */
block_index_t phospherus_createInode(Phospherus_Allocator* allocator, size_t blockSize);

/**
 * @brief Delete the inode on a block, will fail if the inode is still opened
 * 
 * @param allocator Allocator
 * @param inodeBlock Block index where the inode is located
 * @return Is the inode be deleted successfully 
 */
bool phospherus_deleteInode(Phospherus_Allocator* allocator, block_index_t inodeBlock);

/**
 * @brief Resize the inode, will truncate the data if the removed part contains the data
 * 
 * @param inodeDesc Desc to the inode
 * @param newBlockSize New size to resize to
 * @return Is the inode be resized successfully
 */
bool phospherus_resizeInode(iNodeDesc* inodeDesc, size_t newBlockSize);

/**
 * @brief Open a inode through on the given block
 * 
 * @param allocator Allocator
 * @param iNodeBlock Block where the inode is located
 * @return iNodeDesc* Desc to the inode opened
 */
iNodeDesc* phospherus_openInode(Phospherus_Allocator* allocator, block_index_t iNodeBlock);

/**
 * @brief Close the inode opened, please note that once the desc is closed, it should not be used anymore
 * 
 * @param inodeDesc Desc to the inode
 */
void phospherus_closeInode(iNodeDesc* inodeDesc);

/**
 * @brief Read the data inside the inode
 * 
 * @param inodeDesc Desc to the inode
 * @param buffer Buffer to write to
 * @param blockIndexInNode First block to read, start from the beginning of the inode
 * @param blockSize How many block(s) to read
 */
void phospherus_readInodeBlocks(iNodeDesc* inodeDesc, void* buffer, size_t blockIndexInNode, size_t blockSize);

/**
 * @brief Write the data inside the inode
 * 
 * @param inodeDesc Desc to the inode
 * @param buffer Buffer to read from
 * @param blockIndexInNode First block to write, start from the beginning of the inode
 * @param blockSize How many block(s) to write
 * @return Is the data written to the inode successfully
 */
bool phospherus_writeInodeBlocks(iNodeDesc* inodeDesc, const void* buffer, size_t blockIndexInNode, size_t blockSize);

/**
 * @brief Get the inode write protection
 * 
 * @param inodeDesc Desc to the inode
 * @return Is the inode write protection set
 */
bool phospherus_getInodeWriteProtection(iNodeDesc* inodeDesc);

/**
 * @brief Set the inode write protection, once set, any modification toward the inode will be failed
 * 
 * @param inodeDesc Desc to the inode
 * @param writeProtection Is the write protection set
 */
void phospherus_setInodeWriteProtection(iNodeDesc* inodeDesc, bool writeProtection);

/**
 * @brief Get the size of data inside the inode
 * 
 * @param inodeDesc Desc to the inode
 * @return size_t Size of the data
 */
size_t phospherus_getInodeDataSize(iNodeDesc* inodeDesc);

/**
 * @brief Set the size of data inside the inode
 * 
 * @param inodeDesc Desc to the inode
 * @param dataSize size of the data
 */
void phospherus_setInodeDataSize(iNodeDesc* inodeDesc, size_t dataSize);

/**
 * @brief Get num of blocks connected by the inode
 * 
 * @param inodeDesc Desc to the inode
 * @return size_t Num of blocks connected by the inode
 */
size_t phospherus_getInodeBlockSize(iNodeDesc* inodeDesc);

#endif // __PHOSPHERUS_INODE_H
