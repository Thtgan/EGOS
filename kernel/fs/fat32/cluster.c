#include<fs/fat32/cluster.h>

#include<fs/fat32/fat32.h>
#include<kit/types.h>
#include<kit/util.h>

FAT32ClusterType FAT32_cluster_getType(FAT32info* info, Index32 physicalClusterIndex) {
    if (physicalClusterIndex == 0x00000000) {
        return FAT32_CLUSTER_TYPE_FREE;
    }

    if (0x00000002 <= physicalClusterIndex && physicalClusterIndex < info->clusterNum) {
        return FAT32_CLUSTER_TYPE_ALLOCATERD;
    }

    if (info->clusterNum <= physicalClusterIndex && physicalClusterIndex < 0x0FFFFFF7) {
        return FAT32_CLUSTER_TYPE_RESERVED;
    }

    if (physicalClusterIndex == 0x0FFFFFF7) {
        return FAT32_CLUSTER_TYPE_BAD;
    }

    if (0x0FFFFFF8 <= physicalClusterIndex && physicalClusterIndex < 0x10000000) {
        return FAT32_CLUSTER_TYPE_EOF;
    }

    return FAT32_CLUSTER_TYPE_NOT_CLUSTER;
}

Index32 FAT32_cluster_get(FAT32info* info, Index32 firstCluster, Index32 index) {
    if (FAT32_cluster_getType(info, firstCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return INVALID_INDEX;
    }

    Index32* FAT = info->FAT;
    Index32 ret = firstCluster;
    for (int i = 0; i < index; ++i) {
        ret = PTR_TO_VALUE(32, FAT + ret);
    }
    return ret;
}

Size FAT32_cluster_getChainLength(FAT32info* info, Index32 firstCluster) {
    if (FAT32_cluster_getType(info, firstCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return -1;
    }

    Size ret = 1;
    Index32* FAT = info->FAT;

    Index32 current = firstCluster;
    while (true) {
        current = PTR_TO_VALUE(32, FAT + current);
        if (FAT32_cluster_getType(info, current) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            break;
        }
        ++ret;
    }

    return ret;
}

Index32 FAT32_cluster_allocChain(FAT32info* info, Size length) {
    Index32* FAT = info->FAT;
    Index32 current = info->firstFreeCluster, last = INVALID_INDEX;
    for (int i = 0; i < length; ++i) {
        if (current == FAT32_CLSUTER_END_OF_CHAIN) {
            return INVALID_INDEX;
        }

        last = current;
        current = PTR_TO_VALUE(32, FAT + current);
    }

    Index32 ret = info->firstFreeCluster;
    info->firstFreeCluster = current;
    PTR_TO_VALUE(32, FAT + last) = FAT32_CLSUTER_END_OF_CHAIN;

    return ret;
}

void FAT32_cluster_freeChain(FAT32info* info, Index32 clusterChainFirst) {
    Index32* FAT = info->FAT;
    Index32 current = info->firstFreeCluster, last = INVALID_INDEX;
    while (current != FAT32_CLSUTER_END_OF_CHAIN) {
        last = current;
        current = PTR_TO_VALUE(32, FAT + current);
    }

    PTR_TO_VALUE(32, FAT + last) = info->firstFreeCluster;
    info->firstFreeCluster = clusterChainFirst;
}

Index32 FAT32_cluster_cutChain(FAT32info* info, Index32 cluster) {
    if (FAT32_cluster_getType(info, cluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return -1;
    }

    Index32* FAT = info->FAT;
    Index32 ret = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = FAT32_CLSUTER_END_OF_CHAIN;
    return ret;
}

void FAT32_cluster_insertChain(FAT32info* info, Index32 cluster, Index32 clusterChainToInsert) {
    if (FAT32_cluster_getType(info, cluster) != FAT32_CLUSTER_TYPE_ALLOCATERD || FAT32_cluster_getType(info, clusterChainToInsert) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return;
    }

    Index32* FAT = info->FAT;
    Index32 current = clusterChainToInsert;
    while (PTR_TO_VALUE(32, FAT + current) != FAT32_CLSUTER_END_OF_CHAIN) {
        current = PTR_TO_VALUE(32, FAT + current);
    }

    PTR_TO_VALUE(32, FAT + current) = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = clusterChainToInsert;
}