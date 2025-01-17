#if !defined(__FS_FAT32_FAT32_H)
#define __FS_FAT32_FAT32_H

typedef struct FAT32BPB FAT32BPB;
typedef struct FAT32info FAT32info;

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/superblock.h>
#include<kit/types.h>

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

typedef struct FAT32info {
    RangeN      FATrange;
    Range       dataBlockRange;

    Uint32      clusterNum;

    FAT32BPB*   BPB;
    Index32*    FAT;

    Index32     firstFreeCluster;
} FAT32info;

OldResult fat32_init();

OldResult fat32_checkType(BlockDevice* blockDevice);

OldResult fat32_open(FS* fs, BlockDevice* blockDevice);

OldResult fat32_close(FS* fs);

OldResult fat32_superBlock_flush(SuperBlock* superBlock);

#endif // __FS_FAT32_FAT32_H
