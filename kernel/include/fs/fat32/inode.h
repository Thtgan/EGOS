#if !defined(__FS_FAT32_INODE_H)
#define __FS_FAT32_INODE_H

#include<kit/types.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<fs/superblock.h>

OldResult fat32_iNode_open(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc);

OldResult fat32_iNode_close(SuperBlock* superBlock, iNode* iNode);

#endif // __FS_FAT32_INODE_H
