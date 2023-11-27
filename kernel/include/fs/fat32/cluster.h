#if !defined(__FAT32_CLUSTER_H)
#define __FAT32_CLUSTER_H

#include<fs/fat32/fat32.h>
#include<kit/types.h>

typedef enum {
    FAT32_CLUSTER_TYPE_FREE,
    FAT32_CLUSTER_TYPE_ALLOCATERD,
    FAT32_CLUSTER_TYPE_RESERVED,
    FAT32_CLUSTER_TYPE_BAD,
    FAT32_CLUSTER_TYPE_EOF,
    FAT32_CLUSTER_TYPE_NOT_CLUSTER
} FAT32ClusterType;

#define FAT32_CLSUTER_END_OF_CHAIN 0x0FFFFFFF

FAT32ClusterType FAT32_cluster_getType(FAT32info* info, Index32 cluster);

Index32 FAT32_cluster_get(FAT32info* info, Index32 firstCluster, Index32 index);

Size FAT32_cluster_getChainLength(FAT32info* info, Index32 firstCluster);

Index32 FAT32_cluster_allocChain(FAT32info* info, Size length);

void FAT32_cluster_freeChain(FAT32info* info, Index32 clusterChainFirst);

Index32 FAT32_cluster_cutChain(FAT32info* info, Index32 cluster);

void FAT32_cluster_insertChain(FAT32info* info, Index32 cluster, Index32 clusterChainFirst);

#endif // __FAT32_CLUSTER_H
