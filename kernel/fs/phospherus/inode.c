#include<fs/phospherus/inode.h>

#include<algorithms.h>
#include<blowup.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/allocator.h>
#include<fs/phospherus/phospherus.h>
#include<kit/macro.h>
#include<memory/buffer.h>
#include<memory/malloc.h>
#include<memory/memory.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdio.h>
#include<string.h>
#include<structs/linkedList.h>
#include<system/systemInfo.h>

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
 * @param size Size of the inode
 */
static void __createInode(Allocator* allocator, iNode* inode, block_index_t inodeBlock, size_t size);

/**
 * @brief Destroy the inode, including destroy the blocks belongs to this inode
 * 
 * @param allocator Allocator
 * @param inode Inode to destroy
 */
static void __destroyInode(Allocator* allocator, iNode* inode);

/**
 * @brief Create tables of the inode connected to all the blocks
 * 
 * @param allocator Allocator
 * @param inode Inode to create the tables
 * @param size Num of the blocks contained by the tables
 */
static void __createBlockTables(Allocator* allocator, iNode* inode, size_t size);

/**
 * @brief Create a table connected to the blocks
 * 
 * @param allocator Allocator
 * @param level Level of the table
 * @param size Num of the blocks contained by the table
 * @return block_index_t The index of the block contains the table
 */
static block_index_t __createBlockTable(Allocator* allocator, size_t level, size_t size);

/**
 * @brief Destroy the tables connected to all the blocks
 * 
 * @param allocator Allocator
 * @param inode Inode contains tables to destroy
 */
static void __destroyBlockTables(Allocator* allocator, iNode* inode);

/**
 * @brief Deatroy a table connected to the blocks
 * 
 * @param allocator Allocator
 * @param tableBlock Index of the block contains the table to destroy
 * @param level Level of the table
 */
static void __destoryBlockTable(Allocator* allocator, block_index_t tableBlock, size_t level);

/**
 * @brief Resize the tables
 * 
 * @param allocator Allocator
 * @param inode Inode contains tables to resize
 * @param newSize New size to resize to
 */
static void __resizeBlockTables(Allocator* allocator, iNode* inode, size_t newSize);

/**
 * @brief Resize the table, will truncate the data if the removed part contains the data
 * 
 * @param allocator Allocator
 * @param tableBlock Index of the block contains the table to resize
 * @param level Level of the table
 * @param size Original size of the table
 * @param newSize New size to resize to
 */
static void __resizeBlockTable(Allocator* allocator, block_index_t tableBlock, size_t level, size_t size, size_t newSize);

/**
 * @brief Read the blocks
 * 
 * @param device Block device
 * @param buffer Buffer to write to
 * @param tableIndex Index of the block contains the table
 * @param level Level of the table
 * @param blockIndexInTable First block to read, start from the beginning of the table
 * @param size How many block(s) to read
 */
static void __readBlocks(BlockDevice* device, void* buffer, block_index_t tableIndex, size_t level, size_t blockIndexInTable, size_t size);

/**
 * @brief Write the blocks
 * 
 * @param device Block device
 * @param buffer Buffer to read from
 * @param tableIndex Index of the block contains the table
 * @param level Level of the table
 * @param blockIndexInTable First block to write, start from the beginning of the table
 * @param size How many block(s) to write
 */
static void __writeBlocks(BlockDevice* device, const void* buffer, block_index_t tableIndex, size_t level, size_t blockIndexInTable, size_t size);

/**
 * @brief Check is the allocator has enough blocks to create the inode
 * 
 * @param allocator Allocator
 * @param size Size of the inode
 * @return Is the create possible
 */
static bool __checkCreatePossible(Allocator* allocator, size_t size);

/**
 * @brief Check is the allocator has enough blocks to resize the inode
 * 
 * @param allocator Allocator
 * @param size Old size
 * @param newSize New size to resize to
 * @return Is the resize possible
 */
static bool __checkResizePossible(Allocator* allocator, size_t size, size_t newSize);

/**
 * @brief Estimate a inode in given size will take how many blocks
 * 
 * @param size Size of the inode
 * @return size_t How many blocks will inode take
 */
static size_t __estimateiNodeBlockNum(size_t size);

/**
 * @brief Estimate a table connected to given size of blocks will take how many blocks
 * 
 * @param level Level of the table
 * @param size Size of the table
 * @return size_t How many blocks will table take
 */
static size_t __estimateBlockTableBlockNum(size_t level, size_t size);

/**
 * @brief Search inode from opened inodes
 * 
 * @param inodeBlock Index of the block where inode is located
 * @return iNodeDesc* DEsc to the inode
 */
static iNodeDesc* __searchInodeDesc(block_index_t inodeBlock);

static LinkedList _inodeList;   //Linked list of the opened inodes

void initInode() {
    initLinkedList(&_inodeList);
}

block_index_t createInode(Allocator* allocator, size_t size) {
    if (!__checkCreatePossible(allocator, size)) {  //Counting on the check
        return PHOSPHERUS_NULL;
    }

    BlockDevice* device = allocator->device;
    block_index_t ret = allocateBlock(allocator);

    iNode* inode = allocateBuffer(BUFFER_SIZE_512);
    __createInode(allocator, inode, ret, size);
    device->writeBlocks(device->additionalData, ret, inode, 1);

    releaseBuffer(inode, BUFFER_SIZE_512);

    return ret;
}

bool deleteInode(Allocator* allocator, block_index_t inodeBlock) {
    if (__searchInodeDesc(inodeBlock) != NULL) {    //If opened, cannt be deleted
        return false;
    }

    BlockDevice* device = allocator->device;
    iNode* inode = allocateBuffer(BUFFER_SIZE_512);

    device->readBlocks(device->additionalData, inodeBlock, inode, 1);
    if (inode->signature != SYSTEM_INFO_MAGIC32) {  //Validation failed
        releaseBuffer(inode, BUFFER_SIZE_512);
        return false;
    }

    __destroyInode(allocator, inode);

    releaseBlock(allocator, inodeBlock);

    releaseBuffer(inode, BUFFER_SIZE_512);
    return true;
}

bool resizeInode(Allocator* allocator, iNodeDesc* inodeDesc, size_t newSize) {
    if (inodeDesc->writeProtection) {   //Cannot be modified
        return false;
    }

    iNode* inode = inodeDesc->inode;
    size_t size = inode->size;
    if (__checkResizePossible(allocator, size, newSize)) {
        return false;
    }

    if (size == newSize) {
        return true;
    }

    __resizeBlockTables(allocator, inode, newSize);
    inode->size = newSize;
    BlockDevice* device = allocator->device;
    device->writeBlocks(device->additionalData, inode->blockIndex, inode, 1);
    return true;
}

iNodeDesc* openInode(BlockDevice* device, block_index_t iNodeBlock) {
    iNodeDesc* ret;
    if ((ret = __searchInodeDesc(iNodeBlock)) != NULL) {    //If opened
        linkedListDelete(&ret->node);                       //Move it to the top, cause it might be used again soon
        ++ret->openCnt;
    } else {
        iNode* inode = malloc(sizeof(iNode));
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
    }
    
    linkedListInsertBack(&_inodeList, &ret->node);
    return ret;
}

void closeInode(BlockDevice* device, iNodeDesc* inodeDesc) {
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

void readInodeBlocks(iNodeDesc* inodeDesc, void* buffer, size_t blockIndexInNode, size_t size) {
    iNode* inode = inodeDesc->inode;
    BlockDevice* device = getBlockDeviceByName(inode->deviceName);
    if (device == NULL) {   //Device not found
        blowup(_readInodeBlocksFailInfo);
    }

    void* currentBuffer = buffer;
    size_t blockSum = 0, remainBlocksToRead = size, currentReadBegin = blockIndexInNode;
    for (int level = 0; level < 6 && remainBlocksToRead > 0; ++level) {
        for (int i = 0; i < _levelTableNums[level] && remainBlocksToRead > 0; ++i) {
            block_index_t tableIndex = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level]))[i];
            if (tableIndex == PHOSPHERUS_NULL) {
                blowup(_readInodeBlocksFailInfo);
            }

            size_t afterSum = blockSum + _levelTableSizes[level];

            if (afterSum > currentReadBegin) {  //This round involves the block(s) to read
                size_t 
                    tableReadBegin = umin64(currentReadBegin - blockSum, _levelTableSizes[level] - 1),
                    tableReadSize = umin64(_levelTableSizes[level] - tableReadBegin, remainBlocksToRead);

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

bool writeInodeBlocks(iNodeDesc* inodeDesc, const void* buffer, size_t blockIndexInNode, size_t size) {
    if (inodeDesc->writeProtection) {   //Cannot be modified
        return false;
    }

    iNode* inode = inodeDesc->inode;
    BlockDevice* device = getBlockDeviceByName(inode->deviceName);
    if (device == NULL) {   //Device not found
        blowup(_writeInodeBlocksFailInfo);
    }

    const void* currentBuffer = buffer;
    size_t blockSum = 0, remainBlocksToWrite = size, currentWriteBegin = blockIndexInNode;
    for (int level = 0; level < 6 && remainBlocksToWrite > 0; ++level) {
        for (int i = 0; i < _levelTableNums[level] && remainBlocksToWrite > 0; ++i) {
            block_index_t tableIndex = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level]))[i];
            if (tableIndex == PHOSPHERUS_NULL) {
                blowup(_writeInodeBlocksFailInfo);
            }

            size_t afterSum = blockSum + _levelTableSizes[level];

            if (afterSum > currentWriteBegin) {  //This round involves the block(s) to write
                size_t 
                    tableWriteBegin = umin64(currentWriteBegin - blockSum, _levelTableSizes[level] - 1),
                    tableWriteSize = umin64(_levelTableSizes[level] - tableWriteBegin, remainBlocksToWrite);

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

void setInodeWriteProtection(iNodeDesc* inodeDesc, bool writeProtection) {
    inodeDesc->writeProtection = writeProtection;
}

static void __createInode(Allocator* allocator, iNode* inode, block_index_t inodeBlock, size_t size) {
    BlockDevice* device = allocator->device;
    memset(inode, 0, sizeof(iNode));

    strcpy(inode->deviceName, device->name);
    inode->blockIndex = inodeBlock;
    inode->signature = SYSTEM_INFO_MAGIC32;
    inode->size = size;

    __createBlockTables(allocator, inode, size);
}

static void __destroyInode(Allocator* allocator, iNode* inode) {
    BlockDevice* device = allocator->device;

    __destroyBlockTables(allocator, inode);

    memset(inode, 0, sizeof(iNode));
}

static const char* _createBlockTablesFailedInfo = "Create block tables failed";

static void __createBlockTables(Allocator* allocator, iNode* inode, size_t size) {
    size_t remainBlock = size;
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

static block_index_t __createBlockTable(Allocator* allocator, size_t level, size_t size) {
    block_index_t ret = PHOSPHERUS_NULL;
    if (level == 0) {   //End of the recursion
        ret = allocateBlock(allocator);
    } else {
        BlockDevice* device = allocator->device;
        ret = allocateBlock(allocator);
        if (ret == PHOSPHERUS_NULL) {
            blowup(_createBlockTableFailedInfo);
        }

        IndexTable* table = allocateBuffer(BUFFER_SIZE_512);

        size_t remainBlock = size, subTableSize = _levelTableSizes[level - 1];
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

static void __destroyBlockTables(Allocator* allocator, iNode* inode) {
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

static void __destoryBlockTable(Allocator* allocator, block_index_t tableBlock, size_t level) {
    if (level == 0) {   //End of the recursion
        releaseBlock(allocator, tableBlock);
    } else {
        BlockDevice* device = allocator->device;

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

        releaseBlock(allocator, tableBlock);
    }
}

static const char* _resizeBlockTablesFailedInfo = "Resize block tables failed";

static void __resizeBlockTables(Allocator* allocator, iNode* inode, size_t newSize) {
    size_t size = inode->size;
    if (size == newSize) {
        return;
    }

    if (size < newSize) {   //Expand the inode
        size_t blockSum = 0, currentSize = size;
        for (int level = 0; level < 6 && currentSize < newSize; ++level) {
            for (int i = 0; i < _levelTableNums[level] && currentSize < newSize; ++i) {
                block_index_t* indexPtr = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level])) + i;

                size_t afterSum = blockSum + _levelTableSizes[level];
                if (afterSum > size) {  //At this round, new block need to be added
                    size_t newSubSize = umin64(_levelTableSizes[level], newSize - blockSum);
                    if (blockSum < size) {  //Current table not empty, should use resize
                        if (*indexPtr == PHOSPHERUS_NULL) {
                            blowup(_resizeBlockTablesFailedInfo);
                        }

                        __resizeBlockTable(allocator, *indexPtr, level, size - blockSum, newSubSize);
                        currentSize += (newSubSize - (size - blockSum));
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

        if (currentSize != newSize) {
            blowup(_resizeBlockTablesFailedInfo);
        }
    } else {    //Truncate the inode, data might be lost here
        size_t blockSum = 0, currentSize = size;
        for (int level = 0; level < 6 && currentSize > newSize; ++level) {
            for (int i = 0; i < _levelTableNums[level] && currentSize > newSize; ++i) {
                block_index_t* indexPtr = ((block_index_t*)(((void*)inode) + _levelTableOffsets[level])) + i;

                size_t afterSum = blockSum + _levelTableSizes[level];
                if (afterSum > newSize) {  //At this round, block(s) need to be removed
                    if (*indexPtr == PHOSPHERUS_NULL) {
                        blowup(_resizeBlockTablesFailedInfo);
                    }

                    size_t subSize = umin64(size - blockSum, _levelTableSizes[level]);
                    if (blockSum < newSize) {   //Some the of the block in the table should be retained, use resize
                        __resizeBlockTable(allocator, *indexPtr, level, subSize, newSize - blockSum);
                        currentSize -= (subSize - (newSize - blockSum));
                    } else {    //The whole table is not necessary, destroy it
                        __destoryBlockTable(allocator, *indexPtr, level);
                        *indexPtr = PHOSPHERUS_NULL;
                        currentSize -= subSize;
                    }
                }

                blockSum = afterSum;
            }
        }

        if (currentSize != newSize) {   //Not expected
            blowup(_resizeBlockTablesFailedInfo);
        }
    }
}

static const char* _resizeBlockTableFailedInfo = "Resize block table failed";

static void __resizeBlockTable(Allocator* allocator, block_index_t tableBlock, size_t level, size_t size, size_t newSize) {
    if (level == 0) {
        blowup(_resizeBlockTableFailedInfo);
    }

    if (size == newSize) {
        return;
    }

    BlockDevice* device = allocator->device;
    IndexTable* table = allocateBuffer(BUFFER_SIZE_512);
    device->readBlocks(device->additionalData, tableBlock, table, 1);

    size_t lowerLevelSize = _levelTableSizes[level - 1], currentSize = size;
    if (size < newSize) {   //Expand the table
        if (level == 1) {
            for (int i = currentSize; currentSize < newSize; i++) {
                table->indexes[i] = allocateBlock(allocator);
                if (table->indexes[i] == PHOSPHERUS_NULL) {
                    blowup(_resizeBlockTableFailedInfo);
                }

                ++currentSize;
            }
        } else {
            size_t blockSum = size / lowerLevelSize * lowerLevelSize;
            for (int i = size / lowerLevelSize; currentSize < newSize; i++) {   //i is the index of the first sub table needs to be expanded
                size_t afterSum = blockSum + lowerLevelSize;
            
                if (blockSum < size) {  //Current table not empty, should use resize
                    if (table->indexes[i] == PHOSPHERUS_NULL) {
                        blowup(_resizeBlockTableFailedInfo);
                    }

                    size_t subSize = umin64(currentSize - blockSum, lowerLevelSize), newSubSize = umin64(newSize - blockSum, lowerLevelSize);
                    __resizeBlockTable(allocator, table->indexes[i], level - 1, subSize, newSubSize);
                    currentSize += (newSubSize - subSize);
                } else {    //Current table not empty, create the new table
                    if (table->indexes[i] != PHOSPHERUS_NULL) {
                        blowup(_resizeBlockTableFailedInfo);
                    }

                    size_t newSubSize = umin64(newSize - blockSum, lowerLevelSize);
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
            for (int i = (currentSize - 1) / lowerLevelSize; currentSize > newSize; --i) {
                if (table->indexes[i] == PHOSPHERUS_NULL) {
                    blowup(_resizeBlockTableFailedInfo);
                }

                releaseBlock(allocator, table->indexes[i]);
                table->indexes[i] = PHOSPHERUS_NULL;
                --currentSize;
            }
        } else {
            size_t blockSum = newSize / lowerLevelSize * lowerLevelSize;
            for (int i = newSize / lowerLevelSize; currentSize > newSize; ++i) {   //i is the index of the first sub table needs to be truncated
                if (table->indexes[i] == PHOSPHERUS_NULL) {
                    blowup(_resizeBlockTableFailedInfo);
                }

                size_t afterSum = blockSum + lowerLevelSize;
                if (newSize < afterSum) {   //Some the of the block in the table should be retained, use resize
                    size_t subSize = umin64(currentSize - blockSum, lowerLevelSize), newSubSize = umin64(newSize - blockSum, lowerLevelSize);

                    __resizeBlockTable(allocator, table->indexes[i], level - 1, subSize, newSubSize);
                    currentSize -= (subSize - newSubSize);
                } else {    //The whole table is not necessary, destroy it
                    size_t subSize = umin64(size - blockSum, lowerLevelSize);
                    __destoryBlockTable(allocator, table->indexes[i], level - 1);
                    table->indexes[i] = PHOSPHERUS_NULL;
                    currentSize -= subSize;
                }

                blockSum = afterSum;
            }
        }
    }

    if (currentSize != newSize) {
        blowup(_resizeBlockTableFailedInfo);
    }

    device->writeBlocks(device->additionalData, tableBlock, table, 1);

    releaseBuffer(table, BUFFER_SIZE_512);
}

static void __readBlocks(BlockDevice* device, void* buffer, block_index_t tableIndex, size_t level, size_t blockIndexInTable, size_t size) {
    if (level == 0) {
        device->readBlocks(device->additionalData, tableIndex, buffer, 1);
        return;
    }

    IndexTable* table = allocateBuffer(BUFFER_SIZE_512);
    device->readBlocks(device->additionalData, tableIndex, table, 1);

    void* currentBuffer = buffer;
    size_t subTableSize = _levelTableSizes[level - 1], base = blockIndexInTable / subTableSize * subTableSize, remainBlockToRead = size, currentIndex = blockIndexInTable;
    for (int i = blockIndexInTable / subTableSize; remainBlockToRead > 0; ++i) {
        size_t subBlockIndex = umin64(currentIndex - base, subTableSize - 1), subSize = umin64(subBlockIndex + size, subTableSize) - subBlockIndex;

        __readBlocks(device, currentBuffer, table->indexes[i], level - 1, subBlockIndex, size);

        remainBlockToRead -= subSize;
        currentIndex += subSize;
        currentBuffer += subSize * BLOCK_SIZE;
    }

    releaseBuffer(table, BUFFER_SIZE_512);
}

static void __writeBlocks(BlockDevice* device, const void* buffer, block_index_t tableIndex, size_t level, size_t blockIndexInTable, size_t size) {
    if (level == 0) {
        device->writeBlocks(device->additionalData, tableIndex, buffer, 1);
        return;
    }

    IndexTable* table = allocateBuffer(BUFFER_SIZE_512);
    device->readBlocks(device->additionalData, tableIndex, table, 1);

    const void* currentBuffer = buffer;
    size_t subTableSize = _levelTableSizes[level - 1], base = blockIndexInTable / subTableSize * subTableSize, remainBlockToWrite = size, currentIndex = blockIndexInTable;
    for (int i = blockIndexInTable / subTableSize; remainBlockToWrite > 0; ++i) {
        size_t subBlockIndex = umin64(currentIndex - base, subTableSize - 1), subSize = umin64(subBlockIndex + size, subTableSize) - subBlockIndex;

        __writeBlocks(device, currentBuffer, table->indexes[i], level - 1, subBlockIndex, size);

        remainBlockToWrite -= subSize;
        currentIndex += subSize;
        currentBuffer += subSize * BLOCK_SIZE;
    }

    releaseBuffer(table, BUFFER_SIZE_512);
}

static bool __checkCreatePossible(Allocator* allocator, size_t size) {
    return __estimateiNodeBlockNum(size) <= getFreeBlockNum(allocator);
}

static bool __checkResizePossible(Allocator* allocator, size_t size, size_t newSize) {
    size_t blockNum = __estimateiNodeBlockNum(size), newBlockNum = __estimateiNodeBlockNum(newSize);
    return blockNum >= newBlockNum || (newBlockNum - blockNum) < getFreeBlockNum(allocator);
}

static size_t __estimateiNodeBlockNum(size_t size) {
    size_t remainBlock = size, ret = 1;
    for (int level = 0; level < 6 && remainBlock > 0; ++level) {
        for (int i = 0; i < _levelTableNums[level] && remainBlock > 0; ++i) {
            if (remainBlock >= _levelTableNums[level]) {    //The full table will use prepared counts to accelerate the calculation
                ret += _levelTableFullSizes[level];
                remainBlock -= _levelTableNums[level];
            } else {
                ret += __estimateBlockTableBlockNum(level, remainBlock);
                remainBlock = 0;
            }
        }
    }
}

static size_t __estimateBlockTableBlockNum(size_t level, size_t size) {
    if (level == 0) {
        return 1;
    }

    size_t ret = 1;
    size_t lowerLevelSize = _levelTableSizes[level - 1], remainBlock = size;
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