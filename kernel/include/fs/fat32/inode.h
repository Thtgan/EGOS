#if !defined(__FS_FAT32_INODE_H)
#define __FS_FAT32_INODE_H

typedef struct FAT32Inode FAT32Inode;

#include<kit/types.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<fs/superblock.h>

typedef struct FAT32Inode {
    iNode inode;
    Index32 firstCluster;
} FAT32Inode;

void fat32_iNode_init();

iNodeOperations* fat32_iNode_getOperations();

Size fat32_iNode_getDirectorySizeInByte(iNode* inode);

#endif // __FS_FAT32_INODE_H
