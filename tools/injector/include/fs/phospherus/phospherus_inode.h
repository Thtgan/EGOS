#if !defined(__PHOSPHERUS_INODE_H)
#define __PHOSPHERUS_INODE_H

#include<fs/fileSystem.h>
#include<kit/types.h>

/**
 * @brief Initialize the inode
 * @return iNodeGlobalOperations* Global operations for inodes
 */
iNodeGlobalOperations* phospherusInitInode();

/**
 * @brief Make a block an iNode
 * 
 * @param device Device
 * @param blockIndex Block
 * @param type Type of the iNode
 */
void makeInode(BlockDevice* device, Index64 blockIndex, iNodeType type);

#endif // __PHOSPHERUS_INODE_H
