#if !defined(__PHOSPHERUS_ALLOCATOR)
#define __PHOSPHERUS_ALLOCATOR

#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/blockLinkedList.h>
#include<stdbool.h>
#include<stddef.h>

#define ALLOCATOR_NODE_SIZE_IN_BLOCK 2048

typedef struct {
    BlockDevice* device;
    void* superNode;
} Allocator;

/**
 * @brief Initialize the block allocator
 */
void initAllocator();

/**
 * @brief Check if the block device has the phospherus' super node deployed
 * 
 * @param device Block device
 * @return If the device has super node deployed
 */
bool checkBlockDevice(BlockDevice* device);

/**
 * @brief Deploy the essential data to the device, the block device must have at least one cluster free
 * 
 * @param device Block device to deploy
 * @return Does the deploy succeeded
 */
bool deployAllocator(BlockDevice* device);

/**
 * @brief Load the allocator from the disk
 * 
 * @param allocator Allocator to initialize
 * @param device Block device involved
 */
void loadAllocator(Allocator* allocator, BlockDevice* device);

/**
 * @brief Unload the allocator, make it unavailable for operation
 * 
 * @param allocator Allocator to unload
 */
void deleteAllocator(Allocator* allocator);

/**
 * @brief Allocate a block (512B)
 * 
 * @param allocator Allocator
 * @return block_index_t Block index of the block
 */
block_index_t allocateBlock(Allocator* allocator);

/**
 * @brief Release the allocated block
 * 
 * @param allocator Allocator
 * @param blockIndex Block to release
 */
void releaseBlock(Allocator* allocator, block_index_t blockIndex);

/**
 * @brief Get the num of the free block
 * 
 * @param allocator Allocator
 * @return size_t Num of the free block
 */
size_t getFreeBlockNum(Allocator* allocator);

//TODO: Implement the security check, to avoid a cluster/fragment be released twice

#endif // __PHOSPHERUS_ALLOCATOR

