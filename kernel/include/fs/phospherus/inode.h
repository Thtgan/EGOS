#if !defined(__PHOSPHERUS_INODE_H)
#define __PHOSPHERUS_INODE_H

#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/allocator.h>

typedef struct {
    char deviceName[16];
    block_index_t blockIndex;
    uint32_t signature; //Used to valid the iNode, maybe can be used for encryption
    uint32_t reserved1;
    size_t size;
    block_index_t clusterLevel0Index[8];    //1MB
    block_index_t clusterLevel1Index[4];    //32MB
    block_index_t clusterLevel2Index[2];    //1GB
    block_index_t clusterLevel3Index;       //32GB
    block_index_t clusterLevel4Index;       //2TB
    block_index_t clusterLevel5Index;       //128TB
    block_index_t fragmentTableIndex;
    uint8_t reserved2[328];
} __attribute__((packed)) iNode;

block_index_t createInode(Allocator* allocator, size_t size);

void deleteInode(Allocator* allocator, block_index_t inode);

iNode* openInode(BlockDevice* device, block_index_t iNodeBlock);

void closeInode(BlockDevice* device, iNode* inode);

#endif // __PHOSPHERUS_INODE_H
