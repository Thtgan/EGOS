#if !defined(__FS_DEVFS_DEVFS_H)
#define __FS_DEVFS_DEVFS_H

typedef struct DevfsFScore DevfsFScore;
typedef struct DevfsNodeMetadata DevfsNodeMetadata;

#include<devices/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsNode.h>
#include<fs/devfs/vnode.h>
#include<fs/fscore.h>
#include<kit/types.h>
#include<structs/hashTable.h>

typedef struct DevfsFScore {
    FScore              fsCore;
    HashTable           metadataTable;
#define DEVFS_FSCORE_VNODE_TABLE_CHAIN_NUM  31
    SinglyLinkedList    metadataTableChains[DEVFS_FSCORE_VNODE_TABLE_CHAIN_NUM];
} DevfsFScore;

typedef struct DevfsNodeMetadata {
    HashChainNode   hashNode;
    fsNode*         node;
    Size            sizeInByte;
    Object          pointsTo;    //Data block index or device ID
} DevfsNodeMetadata;

void devfs_init();

bool devfs_checkType(BlockDevice* blockDevice);

void devfs_open(FS* fs, BlockDevice* blockDevice);

void devfs_close(FS* fs);

void devfsFScore_registerMetadata(DevfsFScore* fsCore, ID vnodeID, fsNode* node, Size sizeInByte, Object pointsTo);

void devfsFScore_unregisterMetadata(DevfsFScore* fsCore, ID vnodeID);

DevfsNodeMetadata* devfsFScore_getMetadata(DevfsFScore* fsCore, ID vnodeID);

#endif // __FS_DEVFS_DEVFS_H
