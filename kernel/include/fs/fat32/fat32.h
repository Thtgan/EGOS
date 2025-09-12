#if !defined(__FS_FAT32_FAT32_H)
#define __FS_FAT32_FAT32_H

typedef struct FAT32fscore FAT32fscore;
typedef struct FAT32BPB FAT32BPB;

#include<devices/blockDevice.h>
#include<fs/directoryEntry.h>
#include<fs/fs.h>
#include<fs/fsNode.h>
#include<fs/fscore.h>
#include<kit/types.h>

typedef struct FAT32fscore {
    FScore              fscore;
    
    RangeN              FATrange;
    Range               dataBlockRange;

    Uint32              clusterNum;

    FAT32BPB*           BPB;
    Index32*            FAT;

    Index32             firstFreeCluster;

    #define FAT32_FSCORE_VNODE_TABLE_CHAIN_NUM  31
    HashTable           metadataTableFirstCluster;
    SinglyLinkedList    metadataTableChainsFirstCluster[FAT32_FSCORE_VNODE_TABLE_CHAIN_NUM];
} FAT32fscore;

typedef struct FAT32NodeMetadata {
    HashChainNode   hashNodeFirstCluster;
    vNodeAttribute  vnodeAttribute;
    Size            size;
} FAT32NodeMetadata;

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

void fat32FScore_registerMetadata(FAT32fscore* fscore, Size size, Index64 firstCluster, vNodeAttribute* vnodeAttribute);

void fat32FScore_unregisterMetadata(FAT32fscore* fscore, Index64 firstCluster);

FAT32NodeMetadata* fat32FScore_getMetadata(FAT32fscore* fscore, Index64 firstCluster);

Index32 fat32FScore_createFirstCluster(FAT32fscore* fscore);

#endif // __FS_FAT32_FAT32_H
