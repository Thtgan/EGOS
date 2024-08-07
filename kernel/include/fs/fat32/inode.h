#if !defined(__FAT32_INODE_H)
#define __FAT32_INODE_H

#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>

Result FAT32_iNode_open(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc);

Result fat32_iNode_close(SuperBlock* superBlock, iNode* iNode);

#endif // __FAT32_INODE_H
