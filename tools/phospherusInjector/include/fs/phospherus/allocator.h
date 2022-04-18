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
} Phospherus_Allocator;

/**
 * @brief Initialize the block allocator
 */
void phospherus_initAllocator();

/**
 * @brief Check if the block device has the phospherus' super node deployed
 * 
 * @param device Block device
 * @return If the device has super node deployed
 */
bool phospherus_checkBlockDevice(BlockDevice* device);

/**
 * @brief Deploy the essential data to the device, the block device must have at least one cluster free
 * 
 * @param device Block device to deploy
 * @return Does the deploy succeeded
 */
bool phospherus_deployAllocator(BlockDevice* device);

/**
 * @brief Open the allocator from the disk
 * 
 * @param device Block device involved
 */
Phospherus_Allocator* phospherus_openAllocator(BlockDevice* device);

/**
 * @brief Close the allocator, make it unavailable for operation
 * 
 * @param allocator Allocator to unload
 */
void phospherus_closeAllocator(Phospherus_Allocator* allocator);

/**
 * @brief Allocate a block (512B)
 * 
 * @param allocator Allocator
 * @return block_index_t Block index of the block
 */
block_index_t phospherus_allocateBlock(Phospherus_Allocator* allocator);

/**
 * @brief Release the allocated block
 * 
 * @param allocator Allocator
 * @param blockIndex Block to release
 */
void phospherus_releaseBlock(Phospherus_Allocator* allocator, block_index_t blockIndex);

/**
 * @brief Get the num of the free block
 * 
 * @param allocator Allocator
 * @return size_t Num of the free block
 */
size_t phospherus_getFreeBlockNum(Phospherus_Allocator* allocator);

/**
 * @brief Get block device used by allocator
 * 
 * @param allocator Allocator
 * @return BlockDevice* Block device used by allocator
 */
BlockDevice* phospherus_getAllocatorDevice(Phospherus_Allocator* allocator);

//TODO: Implement the security check, to avoid a cluster/fragment be released twice

#endif // __PHOSPHERUS_ALLOCATOR

