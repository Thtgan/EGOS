#if !defined(__FS_DEVFS_DEVFS_H)
#define __FS_DEVFS_DEVFS_H

typedef struct DevfsSuperBlock DevfsSuperBlock;
typedef struct DevfsInodeIDtableNode DevfsInodeIDtableNode;

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsNode.h>
#include<fs/devfs/blockChain.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef struct DevfsSuperBlock {
    SuperBlock          superBlock;
    DevfsBlockChains    blockChains;
    HashTable           inodeIDtable;
#define DEVFS_SUPERBLOCK_INODE_TABLE_CHAIN_NUM  31
    SinglyLinkedList    inodeIDtableChains[DEVFS_SUPERBLOCK_INODE_TABLE_CHAIN_NUM];
} DevfsSuperBlock;

typedef struct DevfsInodeIDtableNode {
    HashChainNode   hashNode;
    fsNode*         node;
    Size            sizeInByte;
    Object          pointsTo;    //Data block index or device ID
} DevfsInodeIDtableNode;

void devfs_init();

bool devfs_checkType(BlockDevice* blockDevice);

void devfs_open(FS* fs, BlockDevice* blockDevice);

void devfs_close(FS* fs);

void devfsSuperBlock_registerInodeID(DevfsSuperBlock* superBlock, ID inodeID, fsNode* node, Size sizeInByte, Object pointsTo);

void devfsSuperBlock_unregisterInodeID(DevfsSuperBlock* superBlock, ID inodeID);

#endif // __FS_DEVFS_DEVFS_H
