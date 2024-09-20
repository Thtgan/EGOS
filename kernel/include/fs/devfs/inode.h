#if !defined(__FS_DEVFS_INODE_H)
#define __FS_DEVFS_INODE_H

#include<fs/fsStructs.h>
#include<kit/types.h>

Result devfs_iNode_open(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc);

Result devfs_iNode_close(SuperBlock* superBlock, iNode* iNode);

#endif // __FS_DEVFS_INODE_H
