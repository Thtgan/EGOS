#if !defined(__FS_DEVFS_INODE_H)
#define __FS_DEVFS_INODE_H

typedef struct DevfsInode DevfsInode;
typedef struct DevfsDirectoryEntry DevfsDirectoryEntry;

#include<debug.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<structs/string.h>

typedef struct DevfsInode {
    iNode   inode;
    void*   data;
} DevfsInode;

typedef struct DevfsDirectoryEntry {
    String      name;
    Object      pointTo;
    Size        size;
    fsEntryType type;
    ID          inodeID;
} DevfsDirectoryEntry;

DEBUG_ASSERT_COMPILE(sizeof(DevfsDirectoryEntry) == 64);

void devfsDirectoryEntry_initStruct(DevfsDirectoryEntry* entry, ConstCstring name, fsEntryType type, Object pointTo, SuperBlock* superBlock);

iNodeOperations* devfs_iNode_getOperations();

#endif // __FS_DEVFS_INODE_H
