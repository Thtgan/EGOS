#if !defined(__PHOSPHERUS_ALLOCATOR)
#define __PHOSPHERUS_ALLOCATOR

#include<devices/block/blockDevice.h>
#include<kit/types.h>
#include<structs/hashTable.h>

#define ROOT_SUPER_NODE_INDEX   0
#define CLUSTER_BLOCK_SIZE      8

//First free cluster of root super node is reserved for special purpose like root directory
#define RESERVED_BLOCK_DEVICE_INFO      ((ROOT_SUPER_NODE_INDEX + 1) * CLUSTER_BLOCK_SIZE + 0)
#define RESERVED_BLOCK_ROOT_DIRECTORY   ((ROOT_SUPER_NODE_INDEX + 1) * CLUSTER_BLOCK_SIZE + 1)

typedef struct {
    BlockDevice* device;
    void* deviceInfo;
    void* firstFreeSuperNode;
    void* firstFreeBlockStack;
    uint8_t openCnt;
    HashChainNode hashChainNode;
} PhospherusAllocator;

/**
 * @brief Do necessary initializations for allocator
 */
void phospherusInitAllocator();

/**
 * @brief Check if the block device has the phospherus' super node deployed, sets errorcode to indicate error
 * 
 * @param device Block device
 * @return Result If the device has super node deployed
 */
Result phospherusCheckBlockDevice(BlockDevice* device);

/**
 * @brief Deploy the essential data to the device, the block device must have at least one cluster free, sets errorcode to indicate error
 * 
 * @param device Block device to deploy
 * @return Result Result of the operation
 */
Result phospherusDeployAllocator(BlockDevice* device);

/**
 * @brief Open the allocator from the disk, sets errorcode to indicate error
 * 
 * @param device Block device opened, NULL if error happens
 */
PhospherusAllocator* phospherusOpenAllocator(BlockDevice* device);

/**
 * @brief Close the allocator, make it unavailable for operation, sets errorcode to indicate error
 * 
 * @param allocator Allocator to close
 * @return Result Result of the operation
 */
Result phospherusCloseAllocator(PhospherusAllocator* allocator);

/**
 * @brief Allocate a cluster, sets errorcode to indicate error
 * 
 * @param allocator Allocator
 * @return Index64 First index of the cluster, INVALID_INDEX if error happens
 */
Index64 phospherusAllocateCluster(PhospherusAllocator* allocator);

/**
 * @brief Release the allocated cluster
 * 
 * @param allocator Allocator
 * @param cluster First index of the cluster to release
 * @return Result Result of the operation
 */
Result phospherusReleaseCluster(PhospherusAllocator* allocator, Index64 cluster);

/**
 * @brief Get the remaining free cluster num on allocator
 * 
 * @param allocator Allocator
 * @return size_t Num of free cluster
 */
size_t phospherusGetFreeClusterNum(PhospherusAllocator* allocator);

/**
 * @brief Allocate a block, sets errorcode to indicate error
 * 
 * @param allocator Allocator
 * @return Index64 Index of the block, INVALID_INDEX if error happens
 */
Index64 phospherusAllocateBlock(PhospherusAllocator* allocator);

/**
 * @brief Release the allocated block
 * 
 * @param allocator Allocator
 * @param block Index of the block to release
 * @return Result Result of the operation
 */
Result phospherusReleaseBlock(PhospherusAllocator* allocator, Index64 block);

#endif // __PHOSPHERUS_ALLOCATOR

