#if !defined(__FS_FAT32_FAT32_H)
#define __FS_FAT32_FAT32_H

typedef struct FAT32FScore FAT32FScore;
typedef struct FAT32BPB FAT32BPB;

#include<devices/blockDevice.h>
#include<fs/directoryEntry.h>
#include<fs/fs.h>
#include<fs/fsNode.h>
#include<fs/fscore.h>
#include<kit/types.h>

typedef struct FAT32FScore {
    FScore              fsCore;
    
    RangeN              FATrange;
    Range               dataBlockRange;

    Uint32              clusterNum;

    FAT32BPB*           BPB;
    Index32*            FAT;

    Index32             firstFreeCluster;

    HashTable           metadataTableVnodeID;
    #define FAT32_FSCORE_VNODE_TABLE_CHAIN_NUM  31
    SinglyLinkedList    metadataTableChainsVnodeID[FAT32_FSCORE_VNODE_TABLE_CHAIN_NUM];
    HashTable           metadataTableFirstCluster;
    SinglyLinkedList    metadataTableChainsFirstCluster[FAT32_FSCORE_VNODE_TABLE_CHAIN_NUM];
} FAT32FScore;

typedef struct FAT32NodeMetadata {
    HashChainNode   hashNodeVnodeID;
    HashChainNode   hashNodeFirstCluster;
    String          name;
    fsEntryType     type;
    fsNode*         node;
    fsNode*         belongTo;
    vNodeAttribute  vnodeAttribute;
    Size            size;
    bool            isTouched;
} FAT32NodeMetadata;

#define FAT32_NODE_METADATA_GET_VNODE_ID(__METADATA)        ((__METADATA)->hashNodeVnodeID.key)
#define FAT32_NODE_METADATA_GET_FIRST_CLUSTER(__METADATA)   ((__METADATA)->hashNodeFirstCluster.key)

typedef struct FAT32BPB {
    Uint8   jump[3];
    char    oem[8];
    Uint16  bytePerSector;
    Uint8   sectorPerCluster;
    Uint16  reservedSectorNum;
    Uint8   FATnum;
    Uint16  rootDirectoryEntryNum;
    Uint16  sectorNum;
    Uint8   media;
    Uint16  reserved1;
    Uint16  sectorPerTrack;
    Uint16  headNum;
    Uint32  hiddenSectorNum;
    Uint32  sectorNumLarge;
    Uint32  sectorPerFAT;
    Uint16  flags;
    Uint16  version;
    Uint32  rootDirectoryClusterIndex;
    Uint16  FSinfoSectorNum;
    Uint16  backupBootSectorNum;
    Uint8   reserved2[12];
    Uint8   drive;
    Uint8   winNTflags;
    Uint8   signature;
    Uint32  volumeSerialNumber;
    char    label[11];
    char    systemIdentifier[8];
} __attribute__((packed)) FAT32BPB;

void fat32_init();

bool fat32_checkType(BlockDevice* blockDevice);

void fat32_open(FS* fs, BlockDevice* blockDevice);

void fat32_close(FS* fs);

void fat32FScore_registerMetadata(FAT32FScore* fsCore, DirectoryEntry* entry, fsNode* belongTo, Index64 firstCluster, vNodeAttribute* vnodeAttribute);

void fat32FScore_unregisterMetadata(FAT32FScore* fsCore, Index64 firstCluster);

FAT32NodeMetadata* fat32FScore_getMetadataFromVnodeID(FAT32FScore* fsCore, ID vnodeID);

FAT32NodeMetadata* fat32FScore_getMetadataFromFirstCluster(FAT32FScore* fsCore, Index64 firstCluster);

Index32 fat32FScore_createFirstCluster(FAT32FScore* fsCore);

#endif // __FS_FAT32_FAT32_H
