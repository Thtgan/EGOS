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
 * @brief Check if the block device has the phospherus' super node deployed
 * 
 * @param device Block device
 * @return If the device has super node deployed
 */
bool phospherusCheckBlockDevice(BlockDevice* device);

/**
 * @brief Deploy the essential data to the device, the block device must have at least one cluster free
 * 
 * @param device Block device to deploy
 * @return Does the deploy succeeded
 */
bool phospherusDeployAllocator(BlockDevice* device);

/**
 * @brief Open the allocator from the disk
 * 
 * @param device Block device involved
 */
PhospherusAllocator* phospherusOpenAllocator(BlockDevice* device);

/**
 * @brief Close the allocator, make it unavailable for operation
 * 
 * @param allocator Allocator to unload
 */
void phospherusCloseAllocator(PhospherusAllocator* allocator);

/**
 * @brief Allocate a cluster
 * 
 * @param allocator Allocator
 * @return Index64 First index of the cluster, PHOSPHERUS_NULL if allocate failed
 */
Index64 phospherusAllocateCluster(PhospherusAllocator* allocator);

/**
 * @brief Release the allocated cluster
 * 
 * @param allocator Allocator
 * @param cluster First index of the cluster to release
 */
void phospherusReleaseCluster(PhospherusAllocator* allocator, Index64 cluster);

/**
 * @brief Get the remaining free cluster num on allocator
 * 
 * @param allocator Allocator
 * @return size_t Num of free cluster
 */
size_t phospherusGetFreeClusterNum(PhospherusAllocator* allocator);

/**
 * @brief Allocate a block
 * 
 * @param allocator Allocator
 * @return Index64 Index of the block, PHOSPHERUS_NULL if allocate failed
 */
Index64 phospherusAllocateBlock(PhospherusAllocator* allocator);

/**
 * @brief Release the allocated block
 * 
 * @param allocator Allocator
 * @param block Index of the block to release
 */
void phospherusReleaseBlock(PhospherusAllocator* allocator, Index64 block);

#endif // __PHOSPHERUS_ALLOCATOR

