#include<fs/phospherus/phospherus_inode.h>

#include<algorithms.h>
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
 * @brief Create a iNode with given size on device
 * 
 * @param type type of the iNode
 * @return Index64 The index of the block where the iNode is located
 */
static Index64 __createInode(FileSystem* this, iNodeType type);

/**
 * @brief Delete iNode from device
 * 
 * @param iNodeBlock Block index where the inode is located
 * @return Status of the operation
 */
static int __deleteInode(FileSystem* this, Index64 iNodeBlock);

/**
 * @brief Open a inode through on the given block
 * 
 * @param iNodeBlock Block where the inode is located
 * @return iNode* Opened iNode
 */
static iNode* __openInode(FileSystem* this, Index64 iNodeBlock);

/**
 * @brief Close the inode opened
 * 
 * @param iNode iNode to close
 * @return int Status of the operation
 */
static int __closeInode(iNode* iNode);

iNodeGlobalOperations globalOperations = {
    .createInode    = __createInode,
    .deleteInode    = __deleteInode,
    .openInode      = __openInode,
    .closeInode     = __closeInode
};

/**
 * @brief Resize the inode, will truncate the data if the removed part contains the data
 * 
 * @param iNode iNode
 * @param newBlockSize New size to resize to
 * @return int Status of the operation
 */
static int __resize(iNode* iNode, size_t newBlockSize);

/**
 * @brief Read the data inside the iNode
 * 
 * @param iNode iNode
 * @param buffer Buffer to write to
 * @param blockIndexInNode First block to read, start from the beginning of the iNode
 * @param blockSize How many block(s) to read
 * @return int Status of the operation
 */
static int __readBlocks(iNode* iNode, void* buffer, size_t blockIndexInNode, size_t blockSize);

/**
 * @brief Write the data inside the iNode
 * 
 * @param iNode iNode
 * @param buffer Buffer to read from
 * @param blockIndexInNode First block to write, start from the beginning of the iNode
 * @param blockSize How many block(s) to write
 * @return int Status of the operation
 */
static int __writeBlocks(iNode* iNode, const void* buffer, size_t blockIndexInNode, size_t blockSize);

iNodeOperations phospherusInodeOperations = {
    .resize = __resize,
    .readBlocks = __readBlocks,
    .writeBlocks = __writeBlocks
};

/**
 * @brief Resize the cluster
 * 
 * @param allocator Allocator
 * @param clusterTable A pointer stores the index of the table, in convenience for modifying thr table
 * @param oldSize Old size of the table
 * @param newSize New size of the table
 * @param level Level of the table
 * @return int Status of the operation
 */
static int __resizeClusterTable(PhospherusAllocator* allocator, Index64* clusterTable, size_t oldSize, size_t newSize, int level);

/**
 * @brief Deatroy a table connected to the blocks
 * 
 * @param allocator Allocator
 * @param tableBlock Index of the block contains the table to destroy
 * @param level Level of the table
 */
static void __destoryBlockTable(PhospherusAllocator* allocator, Index64 tableBlock, size_t level);

/**
 * @brief Resize the cluster tables
 * 
 * @param allocator Allocator
 * @param clusterTables Cluster tables
 * @param oldClusterSize Old size of the tables
 * @param newClusterSize New size to resize to
 * @return int Status of the operation
 */
static int __resizeClusterTables(PhospherusAllocator* allocator, __PhospherusInodeData* clusterTables, size_t oldClusterSize, size_t newClusterSize);

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
static int __readFromSubClusterTable(BlockDevice* device, void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize);

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
static int __writeToSubClusterTable(BlockDevice* device, const void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize);

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

static HashTable _hashTable;    //Opened iNodes
static SinglyLinkedList _hashChains[64];

iNodeGlobalOperations* phospherusInitInode() {
    initHashTable(&_hashTable, 64, _hashChains, LAMBDA(size_t, (HashTable* this, Object key) {
        return (size_t)key % 61;
    }));

    return &globalOperations;
}

void makeInode(BlockDevice* device, Index64 blockIndex, iNodeType type) {
    RecordOnDevice* record = allocateBuffer(BUFFER_SIZE_512);
    memset(record, 0, BLOCK_SIZE);
    record->dataSize = record->availableBlockSize = 0;
    record->blockTaken = 1;
    record->signature = SYSTEM_INFO_MAGIC64;
    record->type = type;
    record->linkCnt = 0;

    __PhospherusInodeData* data = (__PhospherusInodeData*)record->data;
    memset(data, PHOSPHERUS_NULL, sizeof(__PhospherusInodeData));

    blockDeviceWriteBlocks(device, blockIndex, record, 1);

    releaseBuffer(record, BUFFER_SIZE_512);
}

static Index64 __createInode(FileSystem* this, iNodeType type) {
    BlockDevice* devicePtr = getBlockDeviceByID(this->device);
    PhospherusAllocator* allocator = phospherusOpenAllocator(devicePtr);
    block_index_t ret = phospherusAllocateBlock(allocator);

    if (ret == PHOSPHERUS_NULL) {
        return PHOSPHERUS_NULL;
    }

    makeInode(devicePtr, ret, type);

    return ret;
}

static int __deleteInode(FileSystem* this, Index64 iNodeBlock) {
    if (hashTableFind(&_hashTable, __INODE_ID(this->device, iNodeBlock)) != NULL) {    //If opened, cannt be deleted
        return -1;
    }

    BlockDevice* devicePtr = getBlockDeviceByID(this->device);

    RecordOnDevice* record = allocateBuffer(BUFFER_SIZE_512);
    blockDeviceReadBlocks(devicePtr, iNodeBlock, record, 1);
    if (record->signature != SYSTEM_INFO_MAGIC64) {  //Validation failed
        releaseBuffer(record, BUFFER_SIZE_512);
        return -1;
    }

    PhospherusAllocator* allocator = phospherusOpenAllocator(devicePtr);
    __PhospherusInodeData* data = (__PhospherusInodeData*)record->data;
    __resizeClusterTables(allocator, data, (record->availableBlockSize + CLUSTER_BLOCK_SIZE - 1) / CLUSTER_BLOCK_SIZE, 0);

    memset(record, 0, BLOCK_SIZE);
    blockDeviceWriteBlocks(devicePtr, iNodeBlock, record, 1);
    releaseBuffer(record, BUFFER_SIZE_512);

    return 0;
}

static iNode* __openInode(FileSystem* this, Index64 iNodeBlock) {
    Object iNodeID = __INODE_ID(this->device, iNodeBlock);
    iNode* ret = NULL;
    HashChainNode* node = NULL;
    if ((node = hashTableFind(&_hashTable, __INODE_ID(this->device, iNodeBlock))) != NULL) {    //If opened, return it
        ret = HOST_POINTER(node, iNode, hashChainNode);
        ++ret->openCnt;
    } else {
        ret = kMalloc(sizeof(iNode), MEMORY_TYPE_NORMAL);

        BlockDevice* devicePtr = getBlockDeviceByID(this->device);
        ret->device = devicePtr;
        ret->blockIndex = iNodeBlock;
        ret->openCnt = 1;
        ret->operations = &phospherusInodeOperations;
        ret->entryReference = NULL;
        initHashChainNode(&ret->hashChainNode);

        blockDeviceReadBlocks(devicePtr, iNodeBlock, &ret->onDevice, 1);

        hashTableInsert(&_hashTable, iNodeID, &ret->hashChainNode);
    }

    return ret;
}

static int __closeInode(iNode* iNode) {
    if (iNode->openCnt == 0) {
        return -1;
    }

    --iNode->openCnt;
    if (iNode->openCnt == 0) {
        if (hashTableDelete(&_hashTable, __INODE_ID(iNode->device->deviceID, iNode->blockIndex)) == NULL) {
            return -1;
        }
        kFree(iNode);
    }

    return 0;
}

static int __resize(iNode* iNode, size_t newBlockSize) {
    size_t 
        oldClusterSize = (iNode->onDevice.availableBlockSize + CLUSTER_BLOCK_SIZE - 1) / CLUSTER_BLOCK_SIZE, 
        newClusterSize = (newBlockSize + CLUSTER_BLOCK_SIZE - 1) / CLUSTER_BLOCK_SIZE;
    
    if (oldClusterSize == newClusterSize) {
        return 0;
    }

    BlockDevice* device = iNode->device;
    PhospherusAllocator* allocator = phospherusOpenAllocator(device);

    RecordOnDevice* record = &iNode->onDevice;
    size_t oldBlockTaken = record->blockTaken, newBlockTaken = __estimateiNodeBlockTaken(newClusterSize);
    //This is a rough approximation, but it is fast and guaranteed to be safe
    if (!(oldBlockTaken >= newBlockTaken || (newBlockTaken - oldBlockTaken + CLUSTER_BLOCK_SIZE - 1) / CLUSTER_BLOCK_SIZE < phospherusGetFreeClusterNum(allocator))) {
        return -1;
    }

    __PhospherusInodeData* data = (__PhospherusInodeData*)record->data;
    if (__resizeClusterTables(allocator, data, oldClusterSize, newClusterSize) == -1) {
        return -1;
    }

    record->blockTaken = newBlockTaken;
    record->availableBlockSize = newClusterSize * CLUSTER_BLOCK_SIZE;
    record->dataSize = umin64(record->availableBlockSize, record->dataSize);
    blockDeviceWriteBlocks(device, iNode->blockIndex, &iNode->onDevice, 1);

    return 0;
}

static int __readBlocks(iNode* iNode, void* buffer, size_t blockIndexInNode, size_t blockSize) {
    if (iNode->onDevice.availableBlockSize < blockIndexInNode + blockSize) {
        return -1;
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
            if (tableIndex == PHOSPHERUS_NULL) {
                return -1;
            }

            size_t afterSum = blockSum + levelTableSize;

            if (afterSum > currentReadBegin) {  //This round involves the block(s) to read
                size_t 
                    tableReadBegin = currentReadBegin - blockSum,
                    tableReadSize = umin64(remainBlocksToRead, levelTableSize - tableReadBegin);

                __readFromSubClusterTable(device, currentBuffer, tableIndex, level, tableReadBegin, tableReadSize);

                remainBlocksToRead -= tableReadSize;
                currentBuffer += tableReadSize * BLOCK_SIZE;
                currentReadBegin += tableReadSize;
            }

            blockSum = afterSum;
        }
    }
    
    if (remainBlocksToRead > 0) {  //Not expected
        return -1;
    }

    return 0;
}

static int __writeBlocks(iNode* iNode, const void* buffer, size_t blockIndexInNode, size_t blockSize) {
    if (iNode->onDevice.availableBlockSize < blockIndexInNode + blockSize) {
        return -1;
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
            if (tableIndex == PHOSPHERUS_NULL) {
                return -1;
            }

            size_t afterSum = blockSum + levelTableSize;

            if (afterSum > currentWriteBegin) {  //This round involves the block(s) to write
                size_t 
                    tableWriteBegin = currentWriteBegin - blockSum,
                    tableWriteSize = umin64(levelTableSize - tableWriteBegin, remainBlocksToWrite);

                __writeToSubClusterTable(device, currentBuffer, tableIndex, level, tableWriteBegin, tableWriteSize);

                remainBlocksToWrite -= tableWriteSize;
                currentBuffer += tableWriteSize * BLOCK_SIZE;
                currentWriteBegin += tableWriteSize;
            }

            blockSum = afterSum;
        }
    }

    if (remainBlocksToWrite > 0) {  //Not expected
        return -1;
    }

    return 0;
}

static int __resizeClusterTables(PhospherusAllocator* allocator, __PhospherusInodeData* clusterTables, size_t oldClusterSize, size_t newClusterSize) {
    if (oldClusterSize == newClusterSize) {
        return 0;
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

            if (__resizeClusterTable(allocator, indexPtr, clusterTableOldSize, clusterTableNewSize, level) == -1) {
                return -1;
            }

            currentSize += (clusterTableNewSize - clusterTableOldSize);
            clusterSum = afterSum;
        }
    }

    if (currentSize != newClusterSize) {
        return -1;
    }

    return 0;
}

static int __resizeClusterTable(PhospherusAllocator* allocator, Index64* clusterTable, size_t oldSize, size_t newSize, int level) {
    if (oldSize == newSize) {
        return 0;
    }

    bool isExpand = oldSize < newSize;

    if (isExpand && oldSize == 0) {
        if (level == 0) {
            *clusterTable = phospherusAllocateCluster(allocator);
        } else {
            *clusterTable = phospherusAllocateBlock(allocator);
        }

        if (*clusterTable == PHOSPHERUS_NULL) {
            return -1;
        }
    }

    if (level > 0) {
        size_t lowBound = oldSize, highBound = newSize;
        if (!isExpand) {
            swap64(&lowBound, &highBound);
        }

        __ClusterTable* table = allocateBuffer(BUFFER_SIZE_512);
        BlockDevice* device = allocator->device;
        blockDeviceReadBlocks(device, *clusterTable, table, 1);

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

            if (__resizeClusterTable(allocator, table->indexes + i, subTableOldSize, subTableNewSize, level - 1) == -1) {
                blockDeviceWriteBlocks(device, *clusterTable, table, 1);
                releaseBuffer(table, BUFFER_SIZE_512);

                return -1;
            }

            currentSize += (subTableNewSize - subTableOldSize);
            clusterSum = afterSum;
        }

        blockDeviceWriteBlocks(device, *clusterTable, table, 1);
        releaseBuffer(table, BUFFER_SIZE_512);

        if (currentSize != newSize) {
            return -1;
        }
    }

    if (!isExpand && newSize == 0) {
        if (level == 0) {
            phospherusReleaseCluster(allocator, *clusterTable);
        } else {
            phospherusReleaseBlock(allocator, *clusterTable);
        }

        *clusterTable = PHOSPHERUS_NULL;
    }

    return 0;
}

static int __readFromSubClusterTable(BlockDevice* device, void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize) {
    if (level == 0) {
        blockDeviceReadBlocks(device, tableIndex + blockIndexInTable, buffer, blockSize);
        return 0;
    }

    __ClusterTable* table = allocateBuffer(BUFFER_SIZE_512);
    blockDeviceReadBlocks(device, tableIndex, table, 1);

    void* currentBuffer = buffer;
    size_t levelTableSize = _levelTableSizes[level - 1] * CLUSTER_BLOCK_SIZE, blockSum = blockIndexInTable / levelTableSize * levelTableSize, remainBlockToRead = blockSize, currentIndex = blockIndexInTable;
    for (int i = blockIndexInTable / levelTableSize; remainBlockToRead > 0; ++i) {
        if (table->indexes[i] == PHOSPHERUS_NULL) {
            return -1;
        }

        size_t afterSum = blockSum + levelTableSize;

        //The part not need to read skipped, so no condition check
        size_t subBlockIndex = currentIndex - blockSum, subSize = umin64(remainBlockToRead, levelTableSize - subBlockIndex);
        __readFromSubClusterTable(device, currentBuffer, table->indexes[i], level - 1, subBlockIndex, subSize);

        remainBlockToRead -= subSize;
        currentIndex += subSize;
        currentBuffer += subSize * BLOCK_SIZE;
        blockSum = afterSum;
    }

    releaseBuffer(table, BUFFER_SIZE_512);

    return 0;
}

static int __writeToSubClusterTable(BlockDevice* device, const void* buffer, Index64 tableIndex, size_t level, size_t blockIndexInTable, size_t blockSize) {
    if (level == 0) {
        blockDeviceWriteBlocks(device, tableIndex + blockIndexInTable, buffer, blockSize);
        return 0;
    }

    __ClusterTable* table = allocateBuffer(BUFFER_SIZE_512);
    blockDeviceReadBlocks(device, tableIndex, table, 1);

    const void* currentBuffer = buffer;
    size_t levelTableSize = _levelTableSizes[level - 1] * CLUSTER_BLOCK_SIZE, blockSum = blockIndexInTable / levelTableSize * levelTableSize, remainBlockToWrite = blockSize, currentIndex = blockIndexInTable;
    for (int i = blockIndexInTable / levelTableSize; remainBlockToWrite > 0; ++i) {
        if (table->indexes[i] == PHOSPHERUS_NULL) {
            return -1;
        }

        size_t afterSum = blockSum + levelTableSize;

        //The part not need to write skipped, so no condition check
        size_t subBlockIndex = currentIndex - blockSum, subSize = umin64(remainBlockToWrite, levelTableSize - subBlockIndex);
        __writeToSubClusterTable(device, currentBuffer, table->indexes[i], level - 1, subBlockIndex, subSize);

        remainBlockToWrite -= subSize;
        currentIndex += subSize;
        currentBuffer += subSize * BLOCK_SIZE;
        blockSum = afterSum;
    }

    releaseBuffer(table, BUFFER_SIZE_512);

    return 0;
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
