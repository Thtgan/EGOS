#if !defined(__PHOSPHERUS_ALLOCATOR)
#define __PHOSPHERUS_ALLOCATOR

#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/blockLinkedList.h>

typedef struct {
    //-1 stands for NULL
    uint32_t signature;
    size_t freeClusterNum;
    BlockLinkedListNode freeClusterListNode;
    size_t freeFragmentBlockNum;
    BlockLinkedListNode freeFragmentClusterListNode;
    uint8_t reserved[180];
    size_t freeBlockNumInCluster;
    uint8_t firstFreeBlock;
    uint8_t freeBlockList[255];
} __attribute__((packed)) ClusterNode;

typedef struct {
    BlockDevice* device;
    ClusterNode* superNode;
} Allocator;

bool checkBlockDevice(const BlockDevice* device);

bool deployAllocator(const BlockDevice* device);

void loadAllocator(Allocator* allocator, BlockDevice* device);

void deleteAllocator(Allocator* allocator);

block_index_t allocateCluster(Allocator* allocator);

void releaseCluster(Allocator* allocator, block_index_t clusterNodeIndex);

block_index_t allocateFragmentBlock(Allocator* allocator);

void releaseFragmentBlock(Allocator* allocator, block_index_t fragmentBlock);

//TODO: Implement the security check, to avoid a cluster/fragment be released twice

#endif // __PHOSPHERUS_ALLOCATOR

