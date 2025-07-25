#if !defined(__FS_FAT32_CLUSTER_H)
#define __FS_FAT32_CLUSTER_H

typedef enum {
    FAT32_CLUSTER_TYPE_FREE,
    FAT32_CLUSTER_TYPE_ALLOCATERD,
    FAT32_CLUSTER_TYPE_RESERVED,
    FAT32_CLUSTER_TYPE_BAD,
    FAT32_CLUSTER_TYPE_EOF,
    FAT32_CLUSTER_TYPE_NOT_CLUSTER
} FAT32ClusterType;

#include<fs/fat32/fat32.h>
#include<kit/types.h>

#define FAT32_CLSUTER_END_OF_CHAIN 0x0FFFFFFF

FAT32ClusterType fat32_getClusterType(FAT32FScore* fsCore, Index32 cluster);

Index32 fat32_getCluster(FAT32FScore* fsCore, Index32 firstCluster, Index32 index);

Index32 fat32_stepCluster(FAT32FScore* fsCore, Index32 firstCluster, Size n, Size* continousRet);

Size fat32_getClusterChainLength(FAT32FScore* fsCore, Index32 firstCluster);

Index32 fat32_allocateClusterChain(FAT32FScore* fsCore, Size length);

void fat32_freeClusterChain(FAT32FScore* fsCore, Index32 clusterChainFirst);

Index32 fat32_cutClusterChain(FAT32FScore* fsCore, Index32 cluster);

void fat32_insertClusterChain(FAT32FScore* fsCore, Index32 cluster, Index32 clusterChainFirst);

#endif // __FS_FAT32_CLUSTER_H
