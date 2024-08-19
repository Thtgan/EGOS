#if !defined(__FAT32_H)
#define __FAT32_H

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<kit/types.h>

typedef struct {
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

typedef struct {
    RangeN      FATrange;
    Range       dataBlockRange;

    Uint32      clusterNum;

    FAT32BPB*   BPB;
    Index32*    FAT;

    Index32     firstFreeCluster;
} FAT32info;

Result fat32_init();

Result fat32_checkType(BlockDevice* blockDevice);

Result fat32_open(FS* fs, BlockDevice* blockDevice);

Result fat32_close(FS* fs);

#endif // __FAT32_H
