#if !defined(__FS_DEVFS_INODE_H)
#define __FS_DEVFS_INODE_H

typedef struct DevfsInode DevfsInode;
typedef struct DevfsDirectoryEntry DevfsDirectoryEntry;

#include<debug.h>
#include<fs/fsEntry.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/types.h>

typedef struct DevfsInode {
    iNode   inode;
    Index8  firstBlock;
} DevfsInode;

typedef struct DevfsDirectoryEntry {
#define DEVFS_DIRECTORY_ENTRY_NAME_LIMIT    31
    char    name[DEVFS_DIRECTORY_ENTRY_NAME_LIMIT + 1];
    union {
        Range       dataRange;
        DeviceID    device;
    };
    Uint8   attribute;
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY FLAG8(0)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN    FLAG8(1)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY FLAG8(2)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_FILE      FLAG8(3)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE    FLAG8(4)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY     -1
    Uint32  magic;
#define __DEVFS_DIRECTORY_ENTRY_MAGIC   0xDE7F53A6
    ID inodeID;
} DevfsDirectoryEntry;

DEBUG_ASSERT_COMPILE(sizeof(DevfsDirectoryEntry) == 64);

void devfsDirectoryEntry_initStruct(DevfsDirectoryEntry* entry, SuperBlock* superBlock, ConstCstring name, fsEntryType type, ID deviceID);

iNodeOperations* devfs_iNode_getOperations();

#endif // __FS_DEVFS_INODE_H
