#include<fs/phospherus/inode.h>

#include<algorithms.h>
#include<blowup.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/allocator.h>
#include<fs/phospherus/phospherus.h>
#include<macro.h>
#include<buffer.h>
#include<malloc.h>
#include<memory.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdio.h>
#include<string.h>
#include<linkedList.h>
#include<systemInfo.h>

#define __TABLE_INDEX_NUM               (BLOCK_SIZE / sizeof(block_index_t))

#define __LEVEL_0_NODE_TABLE_SIZE       1llu
#define __LEVEL_1_NODE_TABLE_SIZE       (__LEVEL_0_NODE_TABLE_SIZE * __TABLE_INDEX_NUM)
#define __LEVEL_2_NODE_TABLE_SIZE       (__LEVEL_1_NODE_TABLE_SIZE * __TABLE_INDEX_NUM)
#define __LEVEL_3_NODE_TABLE_SIZE       (__LEVEL_2_NODE_TABLE_SIZE * __TABLE_INDEX_NUM)
#define __LEVEL_4_NODE_TABLE_SIZE       (__LEVEL_3_NODE_TABLE_SIZE * __TABLE_INDEX_NUM)
#define __LEVEL_5_NODE_TABLE_SIZE       (__LEVEL_4_NODE_TABLE_SIZE * __TABLE_INDEX_NUM)

#define __LEVEL_0_NODE_TABLE_NUM        8llu
#define __LEVEL_1_NODE_TABLE_NUM        4llu
#define __LEVEL_2_NODE_TABLE_NUM        2llu
#define __LEVEL_3_NODE_TABLE_NUM        1llu
#define __LEVEL_4_NODE_TABLE_NUM        1llu
#define __LEVEL_5_NODE_TABLE_NUM        1llu

#define __NODE_TABLE_OFFSET(__LEVEL)    offsetof(iNode, MACRO_CONCENTRATE3(blockTables.level, __LEVEL, BlockTable))

#define __LEVEL_0_NODE_TABLE_FULL_SIZE  (__LEVEL_0_NODE_TABLE_SIZE)
#define __LEVEL_1_NODE_TABLE_FULL_SIZE  (__LEVEL_0_NODE_TABLE_FULL_SIZE + __LEVEL_1_NODE_TABLE_SIZE)
#define __LEVEL_2_NODE_TABLE_FULL_SIZE  (__LEVEL_1_NODE_TABLE_FULL_SIZE + __LEVEL_2_NODE_TABLE_SIZE)
#define __LEVEL_3_NODE_TABLE_FULL_SIZE  (__LEVEL_2_NODE_TABLE_FULL_SIZE + __LEVEL_3_NODE_TABLE_SIZE)
#define __LEVEL_4_NODE_TABLE_FULL_SIZE  (__LEVEL_3_NODE_TABLE_FULL_SIZE + __LEVEL_4_NODE_TABLE_SIZE)
#define __LEVEL_5_NODE_TABLE_FULL_SIZE  (__LEVEL_4_NODE_TABLE_FULL_SIZE + __LEVEL_5_NODE_TABLE_SIZE)

static size_t _levelTableSizes[] = {
    __LEVEL_0_NODE_TABLE_SIZE,
    __LEVEL_1_NODE_TABLE_SIZE,
    __LEVEL_2_NODE_TABLE_SIZE,
    __LEVEL_3_NODE_TABLE_SIZE,
    __LEVEL_4_NODE_TABLE_SIZE,
    __LEVEL_5_NODE_TABLE_SIZE
};

static size_t _levelTableNums[] = {
    __LEVEL_0_NODE_TABLE_NUM,
    __LEVEL_1_NODE_TABLE_NUM,
    __LEVEL_2_NODE_TABLE_NUM,
    __LEVEL_3_NODE_TABLE_NUM,
    __LEVEL_4_NODE_TABLE_NUM,
    __LEVEL_5_NODE_TABLE_NUM
};

static size_t _levelTableOffsets[] = {
    __NODE_TABLE_OFFSET(0),
    __NODE_TABLE_OFFSET(1),
    __NODE_TABLE_OFFSET(2),
    __NODE_TABLE_OFFSET(3),
    __NODE_TABLE_OFFSET(4),
    __NODE_TABLE_OFFSET(5),
};

static size_t _levelTableFullSizes[] = {
    __LEVEL_0_NODE_TABLE_FULL_SIZE,
    __LEVEL_1_NODE_TABLE_FULL_SIZE,
    __LEVEL_2_NODE_TABLE_FULL_SIZE,
    __LEVEL_3_NODE_TABLE_FULL_SIZE,
    __LEVEL_4_NODE_TABLE_FULL_SIZE,
    __LEVEL_5_NODE_TABLE_FULL_SIZE
};

typedef struct {
    block_index_t indexes[__TABLE_INDEX_NUM];
} __attribute__((packed)) IndexTable;

/**
 * @brief Create the inode, do the necessary initialization
 * 
 * @param allocator Allocator
 * @param inode Inode to initialize
 * @param inodeBlock Index of the block where the inode is located
 * @param blockSize Size of the inode
 * @param blockTaken Num of blocks taken by inode
 */
static void __createInode(Phospherus_Allocator* allocator, iNode* inode, block_index_t inodeBlock, size_t blockSize, size_t blockTaken);

/**
 * @brief Destroy the inode, including destroy the blocks belongs to this inode
 * 
 * @param allocator Allocator
 * @param inode Inode to destroy
 */
static void __destroyInode(Phospherus_Allocator* allocator, iNode* inode);

/**
 * @brief Create tables of the inode connected to all the blocks
 * 
 * @param allocator Allocator
 * @param inode Inode to create the tables
 * @param blockSize Num of the blocks contained by the tables
 */
static void __createBlockTables(Phospherus_Allocator* allocator, iNode* inode, size_t blockSize);

/**
 * @brief Create a table connected to the blocks
 * 
 * @param allocator Allocator
 * @param level Level of the table
 * @param blockSize Num of the blocks contained by the table
 * @return block_index_t The index of the block contains the table
 */
static block_index_t __createBlockTable(Phospherus_Allocator* allocator, size_t level, size_t blockSize);

/**
 * @brief Destroy the tables connected to all the blocks
 * 
 * @param allocator Allocator
 * @param inode Inode contains tables to destroy
 */
static void __destroyBlockTables(Phospherus_Allocator* allocator, iNode* inode);

/**
 * @brief Deatroy a table connected to the blocks
 * 
 * @param allocator Allocator
 * @param tableBlock Index of the block contains the table to destroy
 * @param level Level of the table
 */
static void __destoryBlockTable(Phospherus_Allocator* allocator, block_index_t tableBlock, size_t level);

/**
 * @brief Resize the tables
 * 
 * @param allocator Allocator
 * @param inode Inode contains tables to resize
 * @param newBlockSize New size to resize to
 */
static void __resizeBlockTables(Phospherus_Allocator* allocator, iNode* inode, size_t newBlockSize);

/**
 * @brief Resize the table, will truncate the data if the removed part contains the data
 * 
 * @param allocator Allocator
 * @param tableBlock Index of the block contains the table to resize
 * @param level Level of the table
 * @param blockSize Original size of the table
 * @param newBlockSize New size to resize to
 */
static void __resizeBlockTable(Phospherus_Allocator* allocator, block_index_t tableBlock, size_t level, size_t blockSize, size_t newBlockSize);

/**
 * @brief Read the blocks
 * 
 * @param device Block device
 * @param buffer Buffer to write to
 * @param tableIndex Index of the block contains the table
 * @param level Level of the table
 * @param blockIndexInTable First block to read, start from the beginning of the table
 * @param blockSize How many block(s) to read
 */
static void __readBlocks(BlockDevice* device, void* buffer, block_index_t tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize);

/**
 * @brief Write the blocks
 * 
 * @param device Block device
 * @param buffer Buffer to read from
 * @param tableIndex Index of the block contains the table
 * @param level Level of the table
 * @param blockIndexInTable First block to write, start from the beginning of the table
 * @param blockSize How many block(s) to write
 */
static void __writeBlocks(BlockDevice* device, const void* buffer, block_index_t tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize);

/**
 * @brief Estimate a inode in given size will take how many blocks
 * 
 * @param blockSize Size of the inode
 * @return size_t How many blocks will inode take
 */
static size_t __estimateiNodeBlockNum(size_t blockSize);

/**
 * @brief Estimate a table connected to given size of blocks will take how many blocks
 * 
 * @param level Level of the table
 * @param blockSize Size of the table
 * @return size_t How many blocks will table take
 */
static size_t __estimateBlockTableBlockNum(size_t level, size_t blockSize);

/**
 * @brief Search inode from opened inodes
 * 
 * @param inodeBlock Index of the block where inode is located
 * @return iNodeDesc* DEsc to the inode
 */
static iNodeDesc* __searchInodeDesc(block_index_t inodeBlock);

static LinkedList _inodeList;   //Linked list of the opened inodes

void phospherus_initInode() {
    initLinkedList(&_inodeList);
}

block_index_t phospherus_createInode(Phospherus_Allocator* allocator, size_t blockSize) {
    size_t blockTaken = __estimateiNodeBlockNum(blockSize);
    if (blockTaken > phospherus_getFreeBlockNum(allocator)) {  //Counting on the check
        return PHOSPHERUS_NULL;
    }

    BlockDevice* device = phospherus_getAllocatorDevice(allocator);
    block_index_t ret = phospherus_allocateBlock(allocator);

    iNode* inode = allocateBuffer(BUFFER_SIZE_512);
    __createInode(allocator, inode, ret, blockSize, blockTaken);
    device->writeBlocks(device->additionalData, ret, inode, 1);

    releaseBuffer(inode, BUFFER_SIZE_512);

    return ret;
}

bool phospherus_deleteInode(Phospherus_Allocator* allocator, block_index_t inodeBlock) {
    if (__searchInodeDesc(inodeBlock) != NULL) {    //If opened, cannt be deleted
        return false;
    }

    BlockDevice* device = phospherus_getAllocatorDevice(allocator);
    iNode* inode = allocateBuffer(BUFFER_SIZE_512);

    device->readBlocks(device->additionalData, inodeBlock, inode, 1);
    if (inode->signature != SYSTEM_INFO_MAGIC32) {  //Validation failed
        releaseBuffer(inode, BUFFER_SIZE_512);
        return false;
    }

    __destroyInode(allocator, inode);
    device->writeBlocks(device->additionalData, inodeBlock, inode, 1);

    phospherus_releaseBlock(allocator, inodeBlock);

    releaseBuffer(inode, BUFFER_SIZE_512);
    return true;
}

bool phospherus_resizeInode(iNodeDesc* inodeDesc, size_t newBlockSize) {
    if (inodeDesc->writeProtection) {   //Cannot be modified
        return false;
    }

    Phospherus_Allocator* allocator = inodeDesc->allocator;
    iNode* inode = inodeDesc->inode;
    size_t blockTaken = inode->blockTaken, newBlockTaken = __estimateiNodeBlockNum(newBlockSize);
    if (!(blockTaken >= newBlockTaken || (newBlockTaken - blockTaken) < phospherus_getFreeBlockNum(allocator))) {
        return false;
    }

    if (inode->blockSize == newBlockSize) {
        return true;
    }

    __resizeBlockTables(allocator, inode, newBlockSize);
    inode->blockSize = newBlockSize;
    BlockDevice* device = phospherus_getAllocatorDevice(allocator);
    device->writeBlocks(device->additionalData, inode->blockIndex, inode, 1);
    return true;
}

iNodeDesc* phospherus_openInode(Phospherus_Allocator* allocator, block_index_t iNodeBlock) {
    iNodeDesc* ret;
    if ((ret = __searchInodeDesc(iNodeBlock)) != NULL) {    //If opened
        linkedListDelete(&ret->node);                       //Move it to the top, cause it might be used again soon
        ++ret->openCnt;
    } else {
        iNode* inode = malloc(sizeof(iNode));
        BlockDevice* device = phospherus_getAllocatorDevice(allocator);
        device->readBlocks(device->additionalData, iNodeBlock, inode, 1);
        if (inode->signature != SYSTEM_INFO_MAGIC32) {      //Validation failed
            free(inode);
            return NULL;
        }

        ret = malloc(sizeof(iNodeDesc));
        initLinkedListNode(&ret->node);
        ret->inode = inode;
        ret->openCnt = 1;
        ret->writeProtection = false;
        ret->allocator = allocator;
    }
    
    linkedListInsertBack(&_inodeList, &ret->node);
    return ret;
}

void phospherus_closeInode(iNodeDesc* inodeDesc) {
    if (inodeDesc->openCnt == 0) {  //If the closed inode is closed again
        blowup("A closed inode is begin closed, which is not excepted!");
    }

    --inodeDesc->openCnt;
    if (inodeDesc->openCnt == 0) {
        linkedListDelete(&inodeDesc->node);
        free(inodeDesc->inode);
        inodeDesc->inode = NULL;
        free(inodeDesc);
    }
}

static const char* _readInodeBlocksFailInfo = "Read inode blocks failed";

void phospherus_readInodeBlocks(iNodeDesc* inodeDesc, void* buffer, size_t blockIndexInNode, size_t blockSize) {
    iNode* inode = inodeDesc->inode;
    BlockDevice* device = phospherus_getAllocatorDevice(inodeDesc->allocator);

    void* currentBuffer = buffer;
    size_t blockSum = 0, remainBlocksToRead = blockSize, currentReadBegin = blockIndexInNode;
    for (int level = 0; level < 6 && remainBlocksToRead > 0; ++level) {
        size_t levelTableSize = _levelTableSizes[level];
        for (int i = 0; i < _levelTableNums[level] && remainBlocksToRead > 0; ++i) {
            block_index_t tableIndex = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level]))[i];
            if (tableIndex == PHOSPHERUS_NULL) {
                blowup(_readInodeBlocksFailInfo);
            }

            size_t afterSum = blockSum + levelTableSize;

            if (afterSum > currentReadBegin) {  //This round involves the block(s) to read
                size_t 
                    tableReadBegin = umin64(currentReadBegin - blockSum, levelTableSize - 1),
                    tableReadSize = umin64(levelTableSize - tableReadBegin, remainBlocksToRead);

                __readBlocks(device, currentBuffer, tableIndex, level, tableReadBegin, tableReadSize);

                remainBlocksToRead -= tableReadSize;
                currentBuffer += tableReadSize * BLOCK_SIZE;
                currentReadBegin += tableReadSize;
            }

            blockSum = afterSum;
        }
    }
    
    if (remainBlocksToRead > 0) {  //Not expected
        blowup(_readInodeBlocksFailInfo);
    }
}

static const char* _writeInodeBlocksFailInfo = "Write inode blocks failed";

bool phospherus_writeInodeBlocks(iNodeDesc* inodeDesc, const void* buffer, size_t blockIndexInNode, size_t blockSize) {
    if (inodeDesc->writeProtection) {   //Cannot be modified
        return false;
    }

    iNode* inode = inodeDesc->inode;
    BlockDevice* device = phospherus_getAllocatorDevice(inodeDesc->allocator);

    const void* currentBuffer = buffer;
    size_t blockSum = 0, remainBlocksToWrite = blockSize, currentWriteBegin = blockIndexInNode;
    for (int level = 0; level < 6 && remainBlocksToWrite > 0; ++level) {
        size_t levelTableSize = _levelTableSizes[level];
        for (int i = 0; i < _levelTableNums[level] && remainBlocksToWrite > 0; ++i) {
            block_index_t tableIndex = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level]))[i];
            if (tableIndex == PHOSPHERUS_NULL) {
                blowup(_writeInodeBlocksFailInfo);
            }

            size_t afterSum = blockSum + levelTableSize;

            if (afterSum > currentWriteBegin) {  //This round involves the block(s) to write
                size_t 
                    tableWriteBegin = umin64(currentWriteBegin - blockSum, levelTableSize - 1),
                    tableWriteSize = umin64(levelTableSize - tableWriteBegin, remainBlocksToWrite);

                __writeBlocks(device, currentBuffer, tableIndex, level, tableWriteBegin, tableWriteSize);

                remainBlocksToWrite -= tableWriteSize;
                currentBuffer += tableWriteSize * BLOCK_SIZE;
                currentWriteBegin += tableWriteSize;
            }

            blockSum = afterSum;
        }
    }
    
    if (remainBlocksToWrite > 0) {  //Not expected
        blowup(_writeInodeBlocksFailInfo);
    }

    return true;
}

bool phospherus_getInodeWriteProtection(iNodeDesc* inodeDesc) {
    return inodeDesc->writeProtection;
}

void phospherus_setInodeWriteProtection(iNodeDesc* inodeDesc, bool writeProtection) {
    inodeDesc->writeProtection = writeProtection;
}

size_t phospherus_getInodeDataSize(iNodeDesc* inodeDesc) {
    return inodeDesc->inode->dataSize;
}

void phospherus_setInodeDataSize(iNodeDesc* inodeDesc, size_t dataSize) {
    BlockDevice* device = phospherus_getAllocatorDevice(inodeDesc->allocator);
    inodeDesc->inode->dataSize = dataSize;
    device->writeBlocks(device->additionalData, inodeDesc->inode->blockIndex, inodeDesc->inode, 1);
}

size_t phospherus_getInodeBlockSize(iNodeDesc* inodeDesc) {
    return inodeDesc->inode->blockSize;
}

static void __createInode(Phospherus_Allocator* allocator, iNode* inode, block_index_t inodeBlock, size_t blockSize, size_t blockTaken) {
    BlockDevice* device = phospherus_getAllocatorDevice(allocator);
    memset(inode, 0, sizeof(iNode));

    strcpy(inode->deviceName, device->name);
    inode->blockIndex = inodeBlock;
    inode->signature = SYSTEM_INFO_MAGIC32;
    inode->blockSize = blockSize;
    inode->blockTaken = blockTaken;
    inode->dataSize = 0;

    __createBlockTables(allocator, inode, blockSize);
}

static void __destroyInode(Phospherus_Allocator* allocator, iNode* inode) {
    BlockDevice* device = phospherus_getAllocatorDevice(allocator);

    __destroyBlockTables(allocator, inode);

    memset(inode, 0, sizeof(iNode));
}

static const char* _createBlockTablesFailedInfo = "Create block tables failed";

static void __createBlockTables(Phospherus_Allocator* allocator, iNode* inode, size_t blockSize) {
    size_t remainBlock = blockSize;
    memset(&inode->blockTables, PHOSPHERUS_NULL, sizeof(inode->blockTables));
    for (int level = 0; level < 6; ++level) {
        for (int i = 0; i < _levelTableNums[level]; ++i) {
            block_index_t* indexPtr = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level])) + i;

            if (remainBlock == 0) {
                *indexPtr = PHOSPHERUS_NULL;
                continue;
            }

            //How many block(s) does the table have
            size_t tableSize = umin64(remainBlock, _levelTableSizes[level]);

            *indexPtr = __createBlockTable(allocator, level, tableSize);

            if (*indexPtr == PHOSPHERUS_NULL) {
                blowup(_createBlockTablesFailedInfo);
            }

            remainBlock -= tableSize;
        }
    }

    if (remainBlock > 0) {  //Not expected
        blowup(_createBlockTablesFailedInfo);
    }
}

static const char* _createBlockTableFailedInfo = "Create block table failed";

static block_index_t __createBlockTable(Phospherus_Allocator* allocator, size_t level, size_t blockSize) {
    block_index_t ret = PHOSPHERUS_NULL;
    if (level == 0) {   //End of the recursion
        ret = phospherus_allocateBlock(allocator);
    } else {
        BlockDevice* device = phospherus_getAllocatorDevice(allocator);
        ret = phospherus_allocateBlock(allocator);
        if (ret == PHOSPHERUS_NULL) {
            blowup(_createBlockTableFailedInfo);
        }

        IndexTable* table = allocateBuffer(BUFFER_SIZE_512);

        size_t remainBlock = blockSize, subTableSize = _levelTableSizes[level - 1];
        memset(table, PHOSPHERUS_NULL, sizeof(IndexTable));
        for (int i = 0; remainBlock > 0 && i < __TABLE_INDEX_NUM; ++i) {
            //How many block(s) does the sub-table have
            size_t subSize = umin64(remainBlock, subTableSize);
            table->indexes[i] = __createBlockTable(allocator, level - 1, subSize);  //Recursive create table

            if (table->indexes[i] == PHOSPHERUS_NULL) {
                blowup(_createBlockTableFailedInfo);
            }

            remainBlock -= subSize;
        }

        if (remainBlock > 0) {  //Not expected
            blowup(_createBlockTableFailedInfo);
        }

        device->writeBlocks(device->additionalData, ret, table, 1);
        releaseBuffer(table, BUFFER_SIZE_512);
    }

    if (ret == PHOSPHERUS_NULL) {
        blowup(_createBlockTableFailedInfo);
    }
    return ret;
}

static void __destroyBlockTables(Phospherus_Allocator* allocator, iNode* inode) {
    for (int level = 0; level < 6; ++level) {
        for (int i = 0; i < _levelTableNums[level]; ++i) {
            block_index_t* tableIndex = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level])) + i;
            if (*tableIndex == PHOSPHERUS_NULL) {   //Meaning following tables not exists (If not, it is not expected)
                return;
            }

            __destoryBlockTable(allocator, *tableIndex, level);
            *tableIndex = PHOSPHERUS_NULL;
        }
    }
}

static void __destoryBlockTable(Phospherus_Allocator* allocator, block_index_t tableBlock, size_t level) {
    if (level == 0) {   //End of the recursion
        phospherus_releaseBlock(allocator, tableBlock);
    } else {
        BlockDevice* device = phospherus_getAllocatorDevice(allocator);

        IndexTable* table = allocateBuffer(BUFFER_SIZE_512);
        device->readBlocks(device->additionalData, tableBlock, table, 1);

        for (int i = 0; i < __TABLE_INDEX_NUM; ++i) {
            if (table->indexes[i] == PHOSPHERUS_NULL) { //Meaning following tables not exists (If not, it is not expected)
                break;
            }

            __destoryBlockTable(allocator, table->indexes[i], level - 1);   //Recursive destruction
            table->indexes[i] = PHOSPHERUS_NULL;
        }

        releaseBuffer(table, BUFFER_SIZE_512);

        phospherus_releaseBlock(allocator, tableBlock);
    }
}

static const char* _resizeBlockTablesFailedInfo = "Resize block tables failed";

static void __resizeBlockTables(Phospherus_Allocator* allocator, iNode* inode, size_t newBlockSize) {
    size_t blockSize = inode->blockSize;
    if (blockSize == newBlockSize) {
        return;
    }

    if (blockSize < newBlockSize) {   //Expand the inode
        size_t blockSum = 0, currentSize = blockSize;
        for (int level = 0; level < 6 && currentSize < newBlockSize; ++level) {
            size_t levelTableSize = _levelTableSizes[level];
            for (int i = 0; i < _levelTableNums[level] && currentSize < newBlockSize; ++i) {
                block_index_t* indexPtr = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level])) + i;

                size_t afterSum = blockSum + levelTableSize;
                if (afterSum > blockSize) {  //At this round, new block need to be added
                    size_t newSubSize = umin64(levelTableSize, newBlockSize - blockSum);
                    if (blockSum < blockSize) {  //Current table not empty, should use resize
                        if (*indexPtr == PHOSPHERUS_NULL) {
                            blowup(_resizeBlockTablesFailedInfo);
                        }

                        __resizeBlockTable(allocator, *indexPtr, level, blockSize - blockSum, newSubSize);
                        currentSize += (newSubSize - (blockSize - blockSum));
                    } else {    //Current table is empty, create the new table
                        if (*indexPtr != PHOSPHERUS_NULL) {
                            blowup(_resizeBlockTablesFailedInfo);
                        }

                        *indexPtr = __createBlockTable(allocator, level, newSubSize);

                        if (*indexPtr == PHOSPHERUS_NULL) {
                            blowup(_resizeBlockTablesFailedInfo);
                        }
                        currentSize += newSubSize;
                    }
                }

                blockSum = afterSum;
            }
        }

        if (currentSize != newBlockSize) {
            blowup(_resizeBlockTablesFailedInfo);
        }
    } else {    //Truncate the inode, data might be lost here
        size_t blockSum = 0, currentSize = blockSize;
        for (int level = 0; level < 6 && currentSize > newBlockSize; ++level) {
            size_t levelTableSize = _levelTableSizes[level];
            for (int i = 0; i < _levelTableNums[level] && currentSize > newBlockSize; ++i) {
                block_index_t* indexPtr = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level])) + i;

                size_t afterSum = blockSum + levelTableSize;
                if (afterSum > newBlockSize) {  //At this round, block(s) need to be removed
                    if (*indexPtr == PHOSPHERUS_NULL) {
                        blowup(_resizeBlockTablesFailedInfo);
                    }

                    size_t subSize = umin64(blockSize - blockSum, levelTableSize);
                    if (blockSum < newBlockSize) {   //Some the of the block in the table should be retained, use resize
                        __resizeBlockTable(allocator, *indexPtr, level, subSize, newBlockSize - blockSum);
                        currentSize -= (subSize - (newBlockSize - blockSum));
                    } else {    //The whole table is not necessary, destroy it
                        __destoryBlockTable(allocator, *indexPtr, level);
                        *indexPtr = PHOSPHERUS_NULL;
                        currentSize -= subSize;
                    }
                }

                blockSum = afterSum;
            }
        }

        if (currentSize != newBlockSize) {   //Not expected
            blowup(_resizeBlockTablesFailedInfo);
        }
    }
}

static const char* _resizeBlockTableFailedInfo = "Resize block table failed";

static void __resizeBlockTable(Phospherus_Allocator* allocator, block_index_t tableBlock, size_t level, size_t blockSize, size_t newBlockSize) {
    if (level == 0) {
        blowup(_resizeBlockTableFailedInfo);
    }

    if (blockSize == newBlockSize) {
        return;
    }

    BlockDevice* device = phospherus_getAllocatorDevice(allocator);
    IndexTable* table = allocateBuffer(BUFFER_SIZE_512);
    device->readBlocks(device->additionalData, tableBlock, table, 1);

    size_t lowerLevelSize = _levelTableSizes[level - 1], currentSize = blockSize;
    if (blockSize < newBlockSize) {   //Expand the table
        if (level == 1) {
            for (int i = currentSize; currentSize < newBlockSize; i++) {
                table->indexes[i] = phospherus_allocateBlock(allocator);
                if (table->indexes[i] == PHOSPHERUS_NULL) {
                    blowup(_resizeBlockTableFailedInfo);
                }

                ++currentSize;
            }
        } else {
            size_t blockSum = blockSize / lowerLevelSize * lowerLevelSize;
            for (int i = blockSize / lowerLevelSize; currentSize < newBlockSize; i++) {   //i is the index of the first sub table needs to be expanded
                size_t afterSum = blockSum + lowerLevelSize;
            
                if (blockSum < blockSize) {  //Current table not empty, should use resize
                    if (table->indexes[i] == PHOSPHERUS_NULL) {
                        blowup(_resizeBlockTableFailedInfo);
                    }

                    size_t subSize = umin64(currentSize - blockSum, lowerLevelSize), newSubSize = umin64(newBlockSize - blockSum, lowerLevelSize);
                    __resizeBlockTable(allocator, table->indexes[i], level - 1, subSize, newSubSize);
                    currentSize += (newSubSize - subSize);
                } else {    //Current table not empty, create the new table
                    if (table->indexes[i] != PHOSPHERUS_NULL) {
                        blowup(_resizeBlockTableFailedInfo);
                    }

                    size_t newSubSize = umin64(newBlockSize - blockSum, lowerLevelSize);
                    table->indexes[i] = __createBlockTable(allocator, level - 1, newSubSize);

                    if (table->indexes[i] == PHOSPHERUS_NULL) {
                        blowup(_resizeBlockTableFailedInfo);
                    }

                    currentSize += newSubSize;
                }
                blockSum = afterSum;
            }
        }
    } else {    //Truncate the table
        if (level == 1) {
            for (int i = (currentSize - 1) / lowerLevelSize; currentSize > newBlockSize; --i) {
                if (table->indexes[i] == PHOSPHERUS_NULL) {
                    blowup(_resizeBlockTableFailedInfo);
                }

                phospherus_releaseBlock(allocator, table->indexes[i]);
                table->indexes[i] = PHOSPHERUS_NULL;
                --currentSize;
            }
        } else {
            size_t blockSum = newBlockSize / lowerLevelSize * lowerLevelSize;
            for (int i = newBlockSize / lowerLevelSize; currentSize > newBlockSize; ++i) {   //i is the index of the first sub table needs to be truncated
                if (table->indexes[i] == PHOSPHERUS_NULL) {
                    blowup(_resizeBlockTableFailedInfo);
                }

                size_t afterSum = blockSum + lowerLevelSize;
                if (newBlockSize < afterSum) {   //Some the of the block in the table should be retained, use resize
                    size_t subSize = umin64(currentSize - blockSum, lowerLevelSize), newSubSize = umin64(newBlockSize - blockSum, lowerLevelSize);

                    __resizeBlockTable(allocator, table->indexes[i], level - 1, subSize, newSubSize);
                    currentSize -= (subSize - newSubSize);
                } else {    //The whole table is not necessary, destroy it
                    size_t subSize = umin64(blockSize - blockSum, lowerLevelSize);
                    __destoryBlockTable(allocator, table->indexes[i], level - 1);
                    table->indexes[i] = PHOSPHERUS_NULL;
                    currentSize -= subSize;
                }

                blockSum = afterSum;
            }
        }
    }

    if (currentSize != newBlockSize) {
        blowup(_resizeBlockTableFailedInfo);
    }

    device->writeBlocks(device->additionalData, tableBlock, table, 1);

    releaseBuffer(table, BUFFER_SIZE_512);
}

static void __readBlocks(BlockDevice* device, void* buffer, block_index_t tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize) {
    if (level == 0) {
        device->readBlocks(device->additionalData, tableIndex, buffer, 1);
        return;
    }

    IndexTable* table = allocateBuffer(BUFFER_SIZE_512);
    device->readBlocks(device->additionalData, tableIndex, table, 1);

    void* currentBuffer = buffer;
    size_t subTableSize = _levelTableSizes[level - 1], blockSum = blockIndexInTable / subTableSize * subTableSize, remainBlockToRead = blockSize, currentIndex = blockIndexInTable;
    for (int i = blockIndexInTable / subTableSize; remainBlockToRead > 0; ++i) {
        size_t subBlockIndex = umin64(currentIndex - blockSum, subTableSize - 1), subSize = umin64(remainBlockToRead, subTableSize - subBlockIndex);

        __readBlocks(device, currentBuffer, table->indexes[i], level - 1, subBlockIndex, subSize);

        remainBlockToRead -= subSize;
        currentIndex += subSize;
        currentBuffer += subSize * BLOCK_SIZE;
        blockSum += subTableSize;
    }

    releaseBuffer(table, BUFFER_SIZE_512);
}

static void __writeBlocks(BlockDevice* device, const void* buffer, block_index_t tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize) {
    if (level == 0) {
        device->writeBlocks(device->additionalData, tableIndex, buffer, 1);
        return;
    }

    IndexTable* table = allocateBuffer(BUFFER_SIZE_512);
    device->readBlocks(device->additionalData, tableIndex, table, 1);

    const void* currentBuffer = buffer;
    size_t subTableSize = _levelTableSizes[level - 1], blockSum = blockIndexInTable / subTableSize * subTableSize, remainBlockToWrite = blockSize, currentIndex = blockIndexInTable;
    for (int i = blockIndexInTable / subTableSize; remainBlockToWrite > 0; ++i) {
        size_t subBlockIndex = umin64(currentIndex - blockSum, subTableSize - 1), subSize = umin64(remainBlockToWrite, subTableSize - subBlockIndex);

        __writeBlocks(device, currentBuffer, table->indexes[i], level - 1, subBlockIndex, subSize);

        remainBlockToWrite -= subSize;
        currentIndex += subSize;
        currentBuffer += subSize * BLOCK_SIZE;
        blockSum += subTableSize;
    }

    releaseBuffer(table, BUFFER_SIZE_512);
}

static size_t __estimateiNodeBlockNum(size_t blockSize) {
    size_t remainBlock = blockSize, ret = 1;
    for (int level = 0; level < 6 && remainBlock > 0; ++level) {
        size_t levelTableSize = _levelTableSizes[level], levelTableFullSize = _levelTableFullSizes[level];
        for (int i = 0; i < _levelTableNums[level] && remainBlock > 0; ++i) {
            if (remainBlock >= levelTableSize) {    //The full table will use prepared counts to accelerate the calculation
                ret += levelTableFullSize;
                remainBlock -= levelTableSize;
            } else {
                ret += __estimateBlockTableBlockNum(level, remainBlock);
                remainBlock = 0;
            }
        }
    }

    return ret;
}

static size_t __estimateBlockTableBlockNum(size_t level, size_t blockSize) {
    if (level == 0) {
        return 1;
    }

    size_t ret = 1;
    size_t lowerLevelSize = _levelTableSizes[level - 1], remainBlock = blockSize;
    for (int i = 0; remainBlock > 0; ++i) {
        if (remainBlock >= lowerLevelSize) {    //The full table will use prepared counts to accelerate the calculation
            ret += _levelTableFullSizes[level - 1];
            remainBlock -= lowerLevelSize;
        } else {
            ret += __estimateBlockTableBlockNum(level - 1, remainBlock);
            remainBlock = 0;
        }
    }

    return ret;
}

static iNodeDesc* __searchInodeDesc(block_index_t inodeBlock) {
    for (LinkedListNode* i = _inodeList.next; i != &_inodeList; i = i->next) {
        iNodeDesc* inode = HOST_POINTER(i, iNodeDesc, node);
        if (inode->inode->blockIndex == inodeBlock) {   //Identify by the block index
            return inode;
        }
    }
    return NULL;
}