#if !defined(__FS_FAT32_FAT32_H)
#define __FS_FAT32_FAT32_H

typedef struct FAT32SuperBlock FAT32SuperBlock;
typedef struct FAT32BPB FAT32BPB;

#include<devices/blockDevice.h>
#include<fs/directoryEntry.h>
#include<fs/fs.h>
#include<fs/fsNode.h>
#include<fs/superblock.h>
#include<kit/types.h>

typedef struct FAT32SuperBlock {
    SuperBlock          superBlock;
    
    RangeN              FATrange;
    Range               dataBlockRange;

    Uint32              clusterNum;

    FAT32BPB*           BPB;
    Index32*            FAT;

    Index32             firstFreeCluster;

    HashTable           metadataTableInodeID;
    #define FAT32_SUPERBLOCK_INODE_TABLE_CHAIN_NUM  31
    SinglyLinkedList    metadataTableChainsInodeID[FAT32_SUPERBLOCK_INODE_TABLE_CHAIN_NUM];
    HashTable           metadataTableFirstCluster;
    SinglyLinkedList    metadataTableChainsFirstCluster[FAT32_SUPERBLOCK_INODE_TABLE_CHAIN_NUM];
} FAT32SuperBlock;

typedef struct FAT32NodeMetadata {
    HashChainNode   hashNodeInodeID;
    HashChainNode   hashNodeFirstCluster;
    String          name;
    fsEntryType     type;
    fsNode*         node;
    fsNode*         belongTo;
    iNodeAttribute  inodeAttribute;
    Size            size;
    bool            isTouched;
} FAT32NodeMetadata;

#define FAT32_NODE_METADATA_GET_INODE_ID(__METADATA)        ((__METADATA)->hashNodeInodeID.key)
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

void fat32SuperBlock_registerMetadata(FAT32SuperBlock* superBlock, DirectoryEntry* entry, fsNode* belongTo, Index64 firstCluster, iNodeAttribute* inodeAttribute);

void fat32SuperBlock_unregisterMetadata(FAT32SuperBlock* superBlock, Index64 firstCluster);

FAT32NodeMetadata* fat32SuperBlock_getMetadataFromInodeID(FAT32SuperBlock* superBlock, ID inodeID);

FAT32NodeMetadata* fat32SuperBlock_getMetadataFromFirstCluster(FAT32SuperBlock* superBlock, Index64 firstCluster);

Index32 fat32SuperBlock_createFirstCluster(FAT32SuperBlock* superBlock);

#endif // __FS_FAT32_FAT32_H
