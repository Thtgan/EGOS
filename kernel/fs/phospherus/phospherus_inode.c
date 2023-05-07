#include<fs/phospherus/phospherus_inode.h>

#include<algorithms.h>
#include<error.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<fs/phospherus/allocator.h>
#include<fs/phospherus/phospherus.h>
#include<kit/macro.h>
#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<system/systemInfo.h>

#define LEVEL_0_CLUSTER_TABLE_NUM   8llu
#define LEVEL_1_CLUSTER_TABLE_NUM   4llu
#define LEVEL_2_CLUSTER_TABLE_NUM   2llu
#define LEVEL_3_CLUSTER_TABLE_NUM   1llu
#define LEVEL_4_CLUSTER_TABLE_NUM   1llu
#define LEVEL_5_CLUSTER_TABLE_NUM   1llu

typedef struct {
    struct {
        Index64 level0ClusterTable[LEVEL_0_CLUSTER_TABLE_NUM];  //4KB     /Table
        Index64 level1ClusterTable[LEVEL_1_CLUSTER_TABLE_NUM];  //256KB   /Table
        Index64 level2ClusterTable[LEVEL_2_CLUSTER_TABLE_NUM];  //16MB    /Table
        Index64 level3ClusterTable[LEVEL_3_CLUSTER_TABLE_NUM];  //1GB     /Table
        Index64 level4ClusterTable[LEVEL_4_CLUSTER_TABLE_NUM];  //64GB    /Table
        Index64 level5ClusterTable[LEVEL_5_CLUSTER_TABLE_NUM];  //4TB     /Table
    } __attribute__((packed)) clusterTables;  //Similiar to unix, use recursive tables to record the positions of all blocks
} __PhospherusInodeData;

#define __TABLE_INDEX_NUM                                   (BLOCK_SIZE / sizeof(Index64))

//How many clusters a cluster table covers
#define __LEVEL_0_CLUSTER_TABLE_SIZE                        1llu
#define __LEVEL_1_CLUSTER_TABLE_SIZE                        (__LEVEL_0_CLUSTER_TABLE_SIZE * __TABLE_INDEX_NUM)
#define __LEVEL_2_CLUSTER_TABLE_SIZE                        (__LEVEL_1_CLUSTER_TABLE_SIZE * __TABLE_INDEX_NUM)
#define __LEVEL_3_CLUSTER_TABLE_SIZE                        (__LEVEL_2_CLUSTER_TABLE_SIZE * __TABLE_INDEX_NUM)
#define __LEVEL_4_CLUSTER_TABLE_SIZE                        (__LEVEL_3_CLUSTER_TABLE_SIZE * __TABLE_INDEX_NUM)
#define __LEVEL_5_CLUSTER_TABLE_SIZE                        (__LEVEL_4_CLUSTER_TABLE_SIZE * __TABLE_INDEX_NUM)

#define __CLUSTER_TABLE_OFFSET(__LEVEL)                     offsetof(__PhospherusInodeData, MACRO_CONCENTRATE3(clusterTables.level, __LEVEL, ClusterTable))

//How many blocks a full cluster table takes (mapped clusters not included)
#define __LEVEL_0_CLUSTER_TABLE_FULL_SIZE                   (__LEVEL_0_CLUSTER_TABLE_SIZE)
#define __LEVEL_1_CLUSTER_TABLE_FULL_SIZE                   (__LEVEL_0_CLUSTER_TABLE_FULL_SIZE + __LEVEL_1_CLUSTER_TABLE_SIZE)
#define __LEVEL_2_CLUSTER_TABLE_FULL_SIZE                   (__LEVEL_1_CLUSTER_TABLE_FULL_SIZE + __LEVEL_2_CLUSTER_TABLE_SIZE)
#define __LEVEL_3_CLUSTER_TABLE_FULL_SIZE                   (__LEVEL_2_CLUSTER_TABLE_FULL_SIZE + __LEVEL_3_CLUSTER_TABLE_SIZE)
#define __LEVEL_4_CLUSTER_TABLE_FULL_SIZE                   (__LEVEL_3_CLUSTER_TABLE_FULL_SIZE + __LEVEL_4_CLUSTER_TABLE_SIZE)
#define __LEVEL_5_CLUSTER_TABLE_FULL_SIZE                   (__LEVEL_4_CLUSTER_TABLE_FULL_SIZE + __LEVEL_5_CLUSTER_TABLE_SIZE)

//How many blocks a full cluster table takes (including mapped clusters)
#define __LEVEL_0_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS     (__LEVEL_0_CLUSTER_TABLE_SIZE * CLUSTER_BLOCK_SIZE)
#define __LEVEL_1_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS     (__LEVEL_0_CLUSTER_TABLE_FULL_SIZE + __LEVEL_1_CLUSTER_TABLE_SIZE * CLUSTER_BLOCK_SIZE)
#define __LEVEL_2_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS     (__LEVEL_1_CLUSTER_TABLE_FULL_SIZE + __LEVEL_2_CLUSTER_TABLE_SIZE * CLUSTER_BLOCK_SIZE)
#define __LEVEL_3_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS     (__LEVEL_2_CLUSTER_TABLE_FULL_SIZE + __LEVEL_3_CLUSTER_TABLE_SIZE * CLUSTER_BLOCK_SIZE)
#define __LEVEL_4_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS     (__LEVEL_3_CLUSTER_TABLE_FULL_SIZE + __LEVEL_4_CLUSTER_TABLE_SIZE * CLUSTER_BLOCK_SIZE)
#define __LEVEL_5_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS     (__LEVEL_4_CLUSTER_TABLE_FULL_SIZE + __LEVEL_5_CLUSTER_TABLE_SIZE * CLUSTER_BLOCK_SIZE)

static size_t _levelTableSizes[] = {
    __LEVEL_0_CLUSTER_TABLE_SIZE,
    __LEVEL_1_CLUSTER_TABLE_SIZE,
    __LEVEL_2_CLUSTER_TABLE_SIZE,
    __LEVEL_3_CLUSTER_TABLE_SIZE,
    __LEVEL_4_CLUSTER_TABLE_SIZE,
    __LEVEL_5_CLUSTER_TABLE_SIZE
};

static size_t _levelTableNums[] = {
    LEVEL_0_CLUSTER_TABLE_NUM,
    LEVEL_1_CLUSTER_TABLE_NUM,
    LEVEL_2_CLUSTER_TABLE_NUM,
    LEVEL_3_CLUSTER_TABLE_NUM,
    LEVEL_4_CLUSTER_TABLE_NUM,
    LEVEL_5_CLUSTER_TABLE_NUM
};

static size_t _levelTableOffsets[] = {
    __CLUSTER_TABLE_OFFSET(0),
    __CLUSTER_TABLE_OFFSET(1),
    __CLUSTER_TABLE_OFFSET(2),
    __CLUSTER_TABLE_OFFSET(3),
    __CLUSTER_TABLE_OFFSET(4),
    __CLUSTER_TABLE_OFFSET(5),
};

static size_t _levelTableFullSizes[] = {
    __LEVEL_0_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS,
    __LEVEL_1_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS,
    __LEVEL_2_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS,
    __LEVEL_3_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS,
    __LEVEL_4_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS,
    __LEVEL_5_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS
};

typedef struct {
    Index64 indexes[__TABLE_INDEX_NUM];
} __attribute__((packed)) __ClusterTable;

#define __INODE_ID(__DEVICE, __INODE_BLOCK) (((__DEVICE) << 48) | (__INODE_BLOCK))

/**
 * @brief Create a iNode with given size on device, sets errorcode to indicate error
 * 
 * @param type type of the iNode
 * @return Index64 The index of the block where the iNode is located, INVALID_INDEX if there is error
 */
static Index64 __createInode(FileSystem* this, iNodeType type);

/**
 * @brief Delete iNode from device, sets errorcode to indicate error
 * 
 * @param iNodeBlock Block index where the inode is located
 * @return Result Result of the operation
 */
static Result __deleteInode(FileSystem* this, Index64 iNodeBlock);

/**
 * @brief Open a inode through on the given block, sets errorcode to indicate error
 * 
 * @param iNodeBlock Block where the inode is located
 * @return iNode* Opened iNode, NULL if there is error
 */
static iNode* __openInode(FileSystem* this, Index64 iNodeBlock);

/**
 * @brief Close the inode opened, sets errorcode to indicate error
 * 
 * @param iNode iNode to close
 * @return Result Result of the operation
 */
static Result __closeInode(iNode* iNode);

iNodeGlobalOperations globalOperations = {
    .createInode    = __createInode,
    .deleteInode    = __deleteInode,
    .openInode      = __openInode,
    .closeInode     = __closeInode
};

/**
 * @brief Resize the inode, will truncate the data if the removed part contains the data, sets errorcode to indicate error
 * 
 * @param iNode iNode
 * @param newBlockSize New size to resize to
 * @return Result Result of the operation
 */
static Result __resize(iNode* iNode, size_t newBlockSize);

/**
 * @brief Read the data inside the iNode, sets errorcode to indicate error
 * 
 * @param iNode iNode
 * @param buffer Buffer to write to
 * @param blockIndexInNode First block to read, start from the beginning of the iNode
 * @param blockSize How many block(s) to read
 * @return Result Result of the operation
 */
static Result __readBlocks(iNode* iNode, void* buffer, size_t blockIndexInNode, size_t blockSize);

/**
 * @brief Write the data inside the iNode, sets errorcode to indicate error
 * 
 * @param iNode iNode
 * @param buffer Buffer to read from
 * @param blockIndexInNode First block to write, start from the beginning of the iNode
 * @param blockSize How many block(s) to write
 * @return Result Result of the operation
 */
static Result __writeBlocks(iNode* iNode, const void* buffer, size_t blockIndexInNode, size_t blockSize);

iNodeOperations phospherusInodeOperations = {
    .resize = __resize,
    .readBlocks = __readBlocks,
    .writeBlocks = __writeBlocks
};

/**
 * @brief Resize the cluster, sets errorcode to indicate error
 * 
 * @param allocator Allocator
 * @param clusterTable A pointer stores the index of the table, in convenience for modifying thr table
 * @param oldSize Old size of the table
 * @param newSize New size of the table
 * @param level Level of the table
 * @return Result Result of the operation
 */
static Result __resizeClusterTable(PhospherusAllocator* allocator, Index64* clusterTable, size_t oldSize, size_t newSize, int level);

/**
 * @brief Deatroy a table connected to the blocks, sets errorcode to indicate error
 * 
 * @param allocator Allocator
 * @param tableBlock Index of the block contains the table to destroy
 * @param level Level of the table
 */
static void __destoryBlockTable(PhospherusAllocator* allocator, Index64 tableBlock, size_t level);

/**
 * @brief Resize the cluster tables, sets errorcode to indicate error
 * 
 * @param allocator Allocator
 * @param clusterTables Cluster tables
 * @param oldClusterSize Old size of the tables
 * @param newClusterSize New size to resize to
 * @return Result Result of the operation
 */
static Result __resizeClusterTables(PhospherusAllocator* allocator, __PhospherusInodeData* clusterTables, size_t oldClusterSize, size_t newClusterSize);

/**
 * @brief Read the blocks, sets errorcode to indicate error
 * 
 * @param device Block device
 * @param buffer Buffer to write to
 * @param tableIndex Index of the block contains the table
 * @param level Level of the table
 * @param blockIndexInTable First block to read, start from the beginning of the table
 * @param blockSize How many block(s) to read
 * @return Result Result of the operation
 */
static Result __readFromSubClusterTable(BlockDevice* device, void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize);

/**
 * @brief Write the blocks, sets errorcode to indicate error
 * 
 * @param device Block device
 * @param buffer Buffer to read from
 * @param tableIndex Index of the block contains the table
 * @param level Level of the table
 * @param blockIndexInTable First block to write, start from the beginning of the table
 * @param blockSize How many block(s) to write
 * @return Result Result of the operation
 */
static Result __writeToSubClusterTable(BlockDevice* device, const void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize);

/**
 * @brief Estimate a inode in given size will take how many blocks
 * 
 * @param clusterSize Size of the inode
 * @return size_t How many blocks will inode take
 */
static size_t __estimateiNodeBlockTaken(size_t clusterSize);

/**
 * @brief Estimate a table connected to given size of clusters will take how many blocks
 * 
 * @param level Level of the table
 * @param clusterSize Size of the table
 * @return size_t How many blocks will table take
 */
static size_t __estimateiNodeSubBlockTaken(size_t level, size_t clusterSize);

static Result __doMakeInode(BlockDevice* device, Index64 blockIndex, iNodeType type, void** recordPtr);

static Index64 __doCreateInode(FileSystem* this, iNodeType type, PhospherusAllocator** allocatorPtr);

static Result __doDeleteInode(FileSystem* this, Index64 iNodeBlock, void** recordPtr, PhospherusAllocator** allocatorPtr);

static Result __doOpenInode(FileSystem* this, Index64 iNodeBlock, iNode** retPtr);

static Result __doResize(iNode* iNode, size_t newBlockSize, PhospherusAllocator** allocatorPtr);

static Result __doResizeClusterTable(PhospherusAllocator* allocator, Index64* clusterTable, size_t oldSize, size_t newSize, int level, void** tablePtr);

static Result __doReadFromSubClusterTable(BlockDevice* device, void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize, void** tablePtr);

static Result __doWriteToSubClusterTable(BlockDevice* device, const void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize, void** tablePtr);

static HashTable _hashTable;    //Opened iNodes
static SinglyLinkedList _hashChains[64];

iNodeGlobalOperations* phospherusInitInode() {
    initHashTable(&_hashTable, 64, _hashChains, LAMBDA(size_t, (HashTable* this, Object key) {
        return (size_t)key % 61;
    }));

    return &globalOperations;
}

Result makeInode(BlockDevice* device, Index64 blockIndex, iNodeType type) {
    void* record = NULL;

    Result ret = __doMakeInode(device, blockIndex, type, &record);

    if (record != NULL) {
        releaseBuffer(record, BUFFER_SIZE_512);
    }

    return ret;
}

static Index64 __createInode(FileSystem* this, iNodeType type) {
    PhospherusAllocator* allocator = NULL;

    Index64 ret = __doCreateInode(this, type, &allocator);

    if (allocator != NULL) {
        if (phospherusCloseAllocator(allocator) == RESULT_FAIL) {
            return INVALID_INDEX;
        }
    }

    return ret;
}

static Result __deleteInode(FileSystem* this, Index64 iNodeBlock) {
    void* record = NULL;
    PhospherusAllocator* allocator = NULL;

    Result ret = __doDeleteInode(this, iNodeBlock, &record, &allocator);

    if (record != NULL) {
        releaseBuffer(record, BUFFER_SIZE_512);
    }

    if (allocator != NULL) {
        if (phospherusCloseAllocator(allocator) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return RESULT_FAIL;
}

static iNode* __openInode(FileSystem* this, Index64 iNodeBlock) {
    iNode* ret = NULL;

    Result res = __doOpenInode(this, iNodeBlock, &ret);

    if (res == RESULT_FAIL) {
        if (ret != NULL) {
            kFree(ret);
            ret = NULL;
        }
    }

    return ret;
}

static Result __closeInode(iNode* iNode) {
    if (iNode->openCnt == 0) {
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return RESULT_FAIL;
    }

    --iNode->openCnt;
    if (iNode->openCnt == 0) {
        if (hashTableDelete(&_hashTable, __INODE_ID(iNode->device->deviceID, iNode->blockIndex)) == NULL) {
            SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
            return RESULT_FAIL;
        }
        kFree(iNode);
    }

    return RESULT_SUCCESS;
}

static Result __resize(iNode* iNode, size_t newBlockSize) {
    PhospherusAllocator* allocator = NULL;

    Result ret = __doResize(iNode, newBlockSize, &allocator);

    if (allocator != NULL) {
        if (phospherusCloseAllocator(allocator) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return ret;
}

static Result __readBlocks(iNode* iNode, void* buffer, size_t blockIndexInNode, size_t blockSize) {
    if (iNode->onDevice.availableBlockSize < blockIndexInNode + blockSize) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return RESULT_FAIL;
    }
    
    BlockDevice* device = iNode->device;

    void* currentBuffer = buffer;
    size_t blockSum = 0, remainBlocksToRead = blockSize, currentReadBegin = blockIndexInNode;
    RecordOnDevice* record = &iNode->onDevice;

    __PhospherusInodeData* data = (__PhospherusInodeData*)record->data;
    for (int level = 0; level < 6 && remainBlocksToRead > 0; ++level) {
        size_t levelTableSize = _levelTableSizes[level] * CLUSTER_BLOCK_SIZE;   //How many blocks does this cluster cover
        for (int i = 0; i < _levelTableNums[level] && remainBlocksToRead > 0; ++i) {
            Index64 tableIndex = ((Index64*)(((void*)data) + _levelTableOffsets[level]))[i];
            if (tableIndex == INVALID_INDEX) {
                SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
                return RESULT_FAIL;
            }

            size_t afterSum = blockSum + levelTableSize;

            if (afterSum > currentReadBegin) {  //This round involves the block(s) to read
                size_t 
                    tableReadBegin = currentReadBegin - blockSum,
                    tableReadSize = umin64(remainBlocksToRead, levelTableSize - tableReadBegin);

                if (__readFromSubClusterTable(device, currentBuffer, tableIndex, level, tableReadBegin, tableReadSize) == RESULT_FAIL) {
                    return RESULT_FAIL;
                }

                remainBlocksToRead -= tableReadSize;
                currentBuffer += tableReadSize * BLOCK_SIZE;
                currentReadBegin += tableReadSize;
            }

            blockSum = afterSum;
        }
    }
    
    if (remainBlocksToRead > 0) {  //Not expected
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __writeBlocks(iNode* iNode, const void* buffer, size_t blockIndexInNode, size_t blockSize) {
    if (iNode->onDevice.availableBlockSize < blockIndexInNode + blockSize) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return RESULT_FAIL;
    }

    BlockDevice* device = iNode->device;

    const void* currentBuffer = buffer;
    size_t blockSum = 0, remainBlocksToWrite = blockSize, currentWriteBegin = blockIndexInNode;
    RecordOnDevice* record = &iNode->onDevice;

    __PhospherusInodeData* data = (__PhospherusInodeData*)record->data;
    for (int level = 0; level < 6 && remainBlocksToWrite > 0; ++level) {
        size_t levelTableSize = _levelTableSizes[level] * CLUSTER_BLOCK_SIZE;   //How many blocks does this cluster cover
        for (int i = 0; i < _levelTableNums[level] && remainBlocksToWrite > 0; ++i) {
            Index64 tableIndex = ((Index64*)(((void*)data) + _levelTableOffsets[level]))[i];
            if (tableIndex == INVALID_INDEX) {
                SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
                return RESULT_FAIL;
            }

            size_t afterSum = blockSum + levelTableSize;

            if (afterSum > currentWriteBegin) {  //This round involves the block(s) to write
                size_t 
                    tableWriteBegin = currentWriteBegin - blockSum,
                    tableWriteSize = umin64(levelTableSize - tableWriteBegin, remainBlocksToWrite);

                if (__writeToSubClusterTable(device, currentBuffer, tableIndex, level, tableWriteBegin, tableWriteSize) == RESULT_FAIL) {
                    return RESULT_FAIL;
                }

                remainBlocksToWrite -= tableWriteSize;
                currentBuffer += tableWriteSize * BLOCK_SIZE;
                currentWriteBegin += tableWriteSize;
            }

            blockSum = afterSum;
        }
    }

    if (remainBlocksToWrite > 0) {  //Not expected
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __resizeClusterTables(PhospherusAllocator* allocator, __PhospherusInodeData* clusterTables, size_t oldClusterSize, size_t newClusterSize) {
    if (oldClusterSize == newClusterSize) {
        return RESULT_SUCCESS;
    }
    bool isExpand = oldClusterSize < newClusterSize;

    size_t lowBound = oldClusterSize, highBound = newClusterSize;
    if (!isExpand) {
        swap64(&lowBound, &highBound);
    }

    size_t clusterSum = 0, currentSize = oldClusterSize;
    for (int level = 0; level < 6 && currentSize != newClusterSize; ++level) {
        size_t levelTableSize = _levelTableSizes[level];

        for (int i = 0; i < _levelTableNums[level] && currentSize != newClusterSize; ++i) {
            Index64* indexPtr = ((Index64*)(((void*)clusterTables) + _levelTableOffsets[level])) + i;
            size_t afterSum = clusterSum + levelTableSize;

            if (afterSum <= lowBound) {
                clusterSum = afterSum;
                continue;
            }

            size_t
                clusterTableOldSize = clusterSum >= lowBound ? 0 : lowBound - clusterSum, 
                clusterTableNewSize = afterSum <= highBound ? levelTableSize : highBound - clusterSum;

            if (!isExpand) {
                swap64(&clusterTableOldSize, &clusterTableNewSize);
            }

            if (__resizeClusterTable(allocator, indexPtr, clusterTableOldSize, clusterTableNewSize, level) == RESULT_FAIL) {
                return RESULT_FAIL;
            }

            currentSize += (clusterTableNewSize - clusterTableOldSize);
            clusterSum = afterSum;
        }
    }

    if (currentSize != newClusterSize) {
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __resizeClusterTable(PhospherusAllocator* allocator, Index64* clusterTable, size_t oldSize, size_t newSize, int level) {
    void* table = NULL;

    Result res = __doResizeClusterTable(allocator, clusterTable, oldSize, newSize, level, &table);

    if (table != NULL) {
        releaseBuffer(table, BUFFER_SIZE_512);
    }
    
    return res;
}

static Result __readFromSubClusterTable(BlockDevice* device, void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize) {
    void* table = NULL;

    Result res = __doReadFromSubClusterTable(device, buffer, tableIndex, level, blockIndexInTable, blockSize, &table);

    if (table != NULL) {
        releaseBuffer(table, BUFFER_SIZE_512);
    }

    return res;
}

static Result __writeToSubClusterTable(BlockDevice* device, const void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize) {
    void* table = NULL;

    Result res = __doWriteToSubClusterTable(device, buffer, tableIndex, level, blockIndexInTable, blockSize, &table);

    if (table != NULL) {
        releaseBuffer(table, BUFFER_SIZE_512);
    }

    return res;
}

static size_t __estimateiNodeBlockTaken(size_t clusterSize) {
    size_t remainCluster = clusterSize, ret = 1;  //1 for iNode itself
    for (int level = 0; level < 6 && remainCluster > 0; ++level) {
        size_t levelTableSize = _levelTableSizes[level], levelTableFullSize = _levelTableFullSizes[level];
        for (int i = 0; i < _levelTableNums[level] && remainCluster > 0; ++i) {
            if (remainCluster >= levelTableSize) {    //The full table will use prepared counts to accelerate the calculation
                ret += levelTableFullSize;
                remainCluster -= levelTableSize;
            } else {
                ret += __estimateiNodeSubBlockTaken(level, remainCluster);
                remainCluster = 0;
            }
        }
    }

    return ret;
}

static size_t __estimateiNodeSubBlockTaken(size_t level, size_t clusterSize) {
    if (level == 0) {
        return __LEVEL_0_CLUSTER_TABLE_FULL_SIZE_WITH_CLUSTERS;
    }

    size_t ret = 1; //1 for table itself
    size_t lowerLevelSize = _levelTableSizes[level - 1], remainCluster = clusterSize;
    for (int i = 0; remainCluster > 0; ++i) {
        if (remainCluster >= lowerLevelSize) {    //The full table will use prepared counts to accelerate the calculation
            ret += _levelTableFullSizes[level - 1];
            remainCluster -= lowerLevelSize;
        } else {
            ret += __estimateiNodeSubBlockTaken(level - 1, remainCluster);
            remainCluster = 0;
        }
    }

    return ret;
}

static Result __doMakeInode(BlockDevice* device, Index64 blockIndex, iNodeType type, void** recordPtr) {
    RecordOnDevice* record = NULL;
    *recordPtr = record = allocateBuffer(BUFFER_SIZE_512);
    if (record == NULL) {
        return RESULT_FAIL;
    }

    memset(record, 0, BLOCK_SIZE);
    record->dataSize = record->availableBlockSize = 0;
    record->blockTaken = 1;
    record->signature = SYSTEM_INFO_MAGIC64;
    record->type = type;
    record->linkCnt = 0;

    __PhospherusInodeData* data = (__PhospherusInodeData*)record->data;
    memset(data, INVALID_INDEX, sizeof(__PhospherusInodeData));

    if (blockDeviceWriteBlocks(device, blockIndex, record, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Index64 __doCreateInode(FileSystem* this, iNodeType type, PhospherusAllocator** allocatorPtr) {
    BlockDevice* device = getBlockDeviceByID(this->device);
    if (device == NULL) {
        return INVALID_INDEX;
    }

    PhospherusAllocator* allocator = NULL;
    *allocatorPtr = allocator = phospherusOpenAllocator(device);
    if (allocator == NULL) {
        return INVALID_INDEX;
    }

    Index64 ret = phospherusAllocateBlock(allocator);
    if (ret == INVALID_INDEX) {
        return INVALID_INDEX;
    }

    if (makeInode(device, ret, type) == RESULT_FAIL) {
        return INVALID_INDEX;
    }

    return ret;
}

static Result __doDeleteInode(FileSystem* this, Index64 iNodeBlock, void** recordPtr, PhospherusAllocator** allocatorPtr) {
    if (hashTableFind(&_hashTable, __INODE_ID(this->device, iNodeBlock)) != NULL) {    //If opened, cannt be deleted
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_ACCESS_DENIED);
        return RESULT_FAIL;
    }

    BlockDevice* device = getBlockDeviceByID(this->device);
    if (device == NULL) {
        return RESULT_FAIL;
    }

    RecordOnDevice* record = NULL;
    *recordPtr = record = allocateBuffer(BUFFER_SIZE_512);
    if (record == NULL || blockDeviceReadBlocks(device, iNodeBlock, record, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (record->signature != SYSTEM_INFO_MAGIC64) {  //Validation failed
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_VERIFIVCATION_FAIL);
        return RESULT_FAIL;
    }

    PhospherusAllocator* allocator = NULL;
    *allocatorPtr = allocator = phospherusOpenAllocator(device);
    if (
        allocator == NULL ||
        __resizeClusterTables(allocator, (__PhospherusInodeData*)record->data, (record->availableBlockSize + CLUSTER_BLOCK_SIZE - 1) / CLUSTER_BLOCK_SIZE, 0) == RESULT_FAIL
        ) {
        return RESULT_FAIL;
    }

    memset(record, 0, BLOCK_SIZE);
    if (blockDeviceWriteBlocks(device, iNodeBlock, record, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __doOpenInode(FileSystem* this, Index64 iNodeBlock, iNode** retPtr) {
    Object iNodeID = __INODE_ID(this->device, iNodeBlock);
    iNode* ret = NULL;
    HashChainNode* node = NULL;
    if ((node = hashTableFind(&_hashTable, __INODE_ID(this->device, iNodeBlock))) != NULL) {    //If opened, return it
        *retPtr = ret = HOST_POINTER(node, iNode, hashChainNode);
        ++ret->openCnt;
    } else {
        *retPtr = ret = kMalloc(sizeof(iNode), MEMORY_TYPE_NORMAL);
        if (ret == NULL) {
            return RESULT_FAIL;
        }

        BlockDevice* devicePtr = getBlockDeviceByID(this->device);
        ret->device = devicePtr;
        ret->blockIndex = iNodeBlock;
        ret->openCnt = 1;
        ret->operations = &phospherusInodeOperations;
        ret->entryReference = NULL;
        initHashChainNode(&ret->hashChainNode);

        if (blockDeviceReadBlocks(devicePtr, iNodeBlock, &ret->onDevice, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (hashTableInsert(&_hashTable, iNodeID, &ret->hashChainNode) == RESULT_FAIL) {
            SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
            return RESULT_FAIL;
        }
    }

    return RESULT_SUCCESS;
}

static Result __doResize(iNode* iNode, size_t newBlockSize, PhospherusAllocator** allocatorPtr) {
    size_t 
        oldClusterSize = (iNode->onDevice.availableBlockSize + CLUSTER_BLOCK_SIZE - 1) / CLUSTER_BLOCK_SIZE, 
        newClusterSize = (newBlockSize + CLUSTER_BLOCK_SIZE - 1) / CLUSTER_BLOCK_SIZE;
    
    if (oldClusterSize == newClusterSize) {
        return RESULT_SUCCESS;
    }

    BlockDevice* device = iNode->device;
    PhospherusAllocator* allocator = NULL;
    *allocatorPtr = allocator = phospherusOpenAllocator(device);

    RecordOnDevice* record = &iNode->onDevice;
    size_t oldBlockTaken = record->blockTaken, newBlockTaken = __estimateiNodeBlockTaken(newClusterSize);
    //This is a rough approximation, but it is fast and guaranteed to be safe
    if (oldBlockTaken < newBlockTaken && (newBlockTaken - oldBlockTaken + CLUSTER_BLOCK_SIZE - 1) / CLUSTER_BLOCK_SIZE > phospherusGetFreeClusterNum(allocator)) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NO_FREE_SPACE);
        return RESULT_FAIL;
    }

    __PhospherusInodeData* data = (__PhospherusInodeData*)record->data;
    if (__resizeClusterTables(allocator, data, oldClusterSize, newClusterSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    record->blockTaken = newBlockTaken;
    record->availableBlockSize = newClusterSize * CLUSTER_BLOCK_SIZE;
    record->dataSize = umin64(record->availableBlockSize, record->dataSize);
    if (blockDeviceWriteBlocks(device, iNode->blockIndex, &iNode->onDevice, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __doResizeClusterTable(PhospherusAllocator* allocator, Index64* clusterTable, size_t oldSize, size_t newSize, int level, void** tablePtr) {
    if (oldSize == newSize) {
        return RESULT_SUCCESS;
    }

    bool isExpand = oldSize < newSize;

    if (isExpand && oldSize == 0) {
        if (level == 0) {
            *clusterTable = phospherusAllocateCluster(allocator);
        } else {
            *clusterTable = phospherusAllocateBlock(allocator);
        }

        if (*clusterTable == INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NO_FREE_SPACE);
            return RESULT_FAIL;
        }
    }

    if (level > 0) {
        size_t lowBound = oldSize, highBound = newSize;
        if (!isExpand) {
            swap64(&lowBound, &highBound);
        }

        BlockDevice* device = allocator->device;

        __ClusterTable* table = NULL;
        *tablePtr = table = allocateBuffer(BUFFER_SIZE_512);
        if (table == NULL || blockDeviceReadBlocks(device, *clusterTable, table, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        size_t lowerLevelTableSize = _levelTableSizes[level - 1];
        size_t clusterSum = (lowBound / lowerLevelTableSize) * lowerLevelTableSize, currentSize = oldSize;
        for (int i = lowBound / lowerLevelTableSize; currentSize != newSize; ++i) {
            size_t afterSum = clusterSum + lowerLevelTableSize;

            size_t
                subTableOldSize = clusterSum >= lowBound ? 0 : lowBound - clusterSum, 
                subTableNewSize = afterSum <= highBound ? lowerLevelTableSize : highBound - clusterSum;

            if (!isExpand) {
                swap64(&subTableOldSize, &subTableNewSize);
            }

            void* recursiveTable = NULL;
            Result recursiveRes = __doResizeClusterTable(allocator, table->indexes + i, subTableOldSize, subTableNewSize, level - 1, &recursiveTable);

            if (recursiveTable != NULL) {
                releaseBuffer(recursiveTable, BUFFER_SIZE_512);
            }

            if (recursiveRes == RESULT_FAIL) {
                return RESULT_FAIL;
            }

            currentSize += (subTableNewSize - subTableOldSize);
            clusterSum = afterSum;
        }

        if (blockDeviceWriteBlocks(device, *clusterTable, table, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (currentSize != newSize) {
            SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
            return RESULT_FAIL;
        }
    }

    if (!isExpand && newSize == 0) {
        Result res = RESULT_SUCCESS;
        if (level == 0) {
            res = phospherusReleaseCluster(allocator, *clusterTable);
        } else {
            res = phospherusReleaseBlock(allocator, *clusterTable);
        }

        if (res == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        *clusterTable = INVALID_INDEX;
    }

    return RESULT_SUCCESS;
}

static Result __doReadFromSubClusterTable(BlockDevice* device, void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize, void** tablePtr) {
    if (level == 0) {
        return blockDeviceReadBlocks(device, tableIndex + blockIndexInTable, buffer, blockSize);
    }

    __ClusterTable* table = NULL;
    *tablePtr = table = allocateBuffer(BUFFER_SIZE_512);
    if (table == NULL || blockDeviceReadBlocks(device, tableIndex, table, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    void* currentBuffer = buffer;
    size_t levelTableSize = _levelTableSizes[level - 1] * CLUSTER_BLOCK_SIZE, blockSum = blockIndexInTable / levelTableSize * levelTableSize, remainBlockToRead = blockSize, currentIndex = blockIndexInTable;
    for (int i = blockIndexInTable / levelTableSize; remainBlockToRead > 0; ++i) {
        if (table->indexes[i] == INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
            return RESULT_FAIL;
        }

        size_t afterSum = blockSum + levelTableSize;

        //The part not need to read skipped, so no condition check
        size_t subBlockIndex = currentIndex - blockSum, subSize = umin64(remainBlockToRead, levelTableSize - subBlockIndex);

        void* recursiveTable = NULL;

        Result recursiveRes = __doReadFromSubClusterTable(device, currentBuffer, table->indexes[i], level - 1, subBlockIndex, subSize, &recursiveTable);

        if (recursiveTable != NULL) {
            releaseBuffer(recursiveTable, BUFFER_SIZE_512);
        }

        if (recursiveRes == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        remainBlockToRead -= subSize;
        currentIndex += subSize;
        currentBuffer += subSize * BLOCK_SIZE;
        blockSum = afterSum;
    }

    return RESULT_SUCCESS;
}

static Result __doWriteToSubClusterTable(BlockDevice* device, const void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize, void** tablePtr) {
    if (level == 0) {
        return blockDeviceWriteBlocks(device, tableIndex + blockIndexInTable, buffer, blockSize);
    }

    __ClusterTable* table = NULL;
    *tablePtr = table = allocateBuffer(BUFFER_SIZE_512);
    if (table == NULL || blockDeviceReadBlocks(device, tableIndex, table, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    const void* currentBuffer = buffer;
    size_t levelTableSize = _levelTableSizes[level - 1] * CLUSTER_BLOCK_SIZE, blockSum = blockIndexInTable / levelTableSize * levelTableSize, remainBlockToWrite = blockSize, currentIndex = blockIndexInTable;
    for (int i = blockIndexInTable / levelTableSize; remainBlockToWrite > 0; ++i) {
        if (table->indexes[i] == INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
            return RESULT_FAIL;
        }

        size_t afterSum = blockSum + levelTableSize;

        //The part not need to write skipped, so no condition check
        size_t subBlockIndex = currentIndex - blockSum, subSize = umin64(remainBlockToWrite, levelTableSize - subBlockIndex);

        void* recursiveTable = NULL;

        Result recursiveRes = __doWriteToSubClusterTable(device, currentBuffer, table->indexes[i], level - 1, subBlockIndex, subSize, &recursiveTable);

        if (recursiveTable != NULL) {
            releaseBuffer(recursiveTable, BUFFER_SIZE_512);
        }

        if (recursiveRes == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        remainBlockToWrite -= subSize;
        currentIndex += subSize;
        currentBuffer += subSize * BLOCK_SIZE;
        blockSum = afterSum;
    }

    return RESULT_SUCCESS;
}