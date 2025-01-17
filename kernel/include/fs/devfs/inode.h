#if !defined(__FS_DEVFS_INODE_H)
#define __FS_DEVFS_INODE_H

#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/types.h>

OldResult devfs_iNode_open(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc);

OldResult devfs_iNode_close(SuperBlock* superBlock, iNode* iNode);

#endif // __FS_DEVFS_INODE_H
