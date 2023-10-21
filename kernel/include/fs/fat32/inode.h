#if !defined(__FAT32_INODE_H)
#define __FAT32_INODE_H

#include<fs/fileSystem.h>
#include<fs/fileSystemEntry.h>
#include<fs/inode.h>

typedef struct {
    Index64 firstCluster;
} FAT32iNodeInfo;

Result FAT32openInode(SuperBlock* superBlock, iNode* iNode, FileSystemEntryDescriptor* entryDescripotor);

Result FAT32closeInode(SuperBlock* superBlock, iNode* iNode);

#endif // __FAT32_INODE_H
