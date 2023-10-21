#if !defined(__FAT32_H)
#define __FAT32_H

#include<devices/block/blockDevice.h>
#include<fs/fileSystem.h>
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

Result FAT32initFileSystem();

Result FAT32deployFileSystem(BlockDevice* device);

bool FAT32checkFileSystem(BlockDevice* device);

FileSystem* FAT32openFileSystem(BlockDevice* device);

Result FAT32closeFileSystem(FileSystem* fs);

typedef enum {
    FAT32_CLUSTER_TYPE_FREE,
    FAT32_CLUSTER_TYPE_ALLOCATERD,
    FAT32_CLUSTER_TYPE_RESERVED,
    FAT32_CLUSTER_TYPE_BAD,
    FAT32_CLUSTER_TYPE_EOF,
    FAT32_CLUSTER_TYPE_NOT_CLUSTER
} FAT32ClusterType;

#define FAT32_END_OF_CLUSTER_CHAIN 0x0FFFFFFF

FAT32ClusterType FAT32getClusterType(FAT32info* info, Index32 cluster);

Index32 FAT32getCluster(FAT32info* info, Index32 firstCluster, Index32 index);

Size FAT32getClusterChainLength(FAT32info* info, Index32 firstCluster);

Index32 FAT32allocateClusterChain(FAT32info* info, Size length);

void FAT32releaseClusterChain(FAT32info* info, Index32 clusterChainFirst);

Index32 FAT32cutClusterChain(FAT32info* info, Index32 cluster);

void FAT32insertClusterChain(FAT32info* info, Index32 cluster, Index32 clusterChainFirst);

#endif // __FAT32_H
