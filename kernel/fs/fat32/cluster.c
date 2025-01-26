#include<fs/fat32/cluster.h>

#include<fs/fat32/fat32.h>
#include<kit/types.h>
#include<kit/util.h>
#include<debug.h>
#include<error.h>

//TODO: These codes seems wrong
FAT32ClusterType fat32_getClusterType(FAT32info* info, Index32 physicalClusterIndex) {
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

Index32 fat32_getCluster(FAT32info* info, Index32 firstCluster, Index32 index) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(info, firstCluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = info->FAT;
    Index32 ret = firstCluster;
    for (int i = 0; i < index; ++i) {
        ret = PTR_TO_VALUE(32, FAT + ret);
        if (fat32_getClusterType(info, ret) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
        }
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX;
}

Size fat32_getClusterChainLength(FAT32info* info, Index32 firstCluster) {
    if (fat32_getClusterType(info, firstCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return 0;
    }

    Size ret = 1;
    Index32* FAT = info->FAT;

    Index32 current = firstCluster;
    while (true) {
        current = PTR_TO_VALUE(32, FAT + current);
        if (fat32_getClusterType(info, current) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            break;
        }
        ++ret;
    }

    return ret;
}

Index32 fat32_allocateClusterChain(FAT32info* info, Size length) {
    Index32* FAT = info->FAT;
    Index32 current = info->firstFreeCluster, last = INVALID_INDEX;
    for (int i = 0; i < length; ++i) {
        if (current == FAT32_CLSUTER_END_OF_CHAIN) {
            ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
        }

        last = current;
        current = PTR_TO_VALUE(32, FAT + current);
    }

    Index32 ret = info->firstFreeCluster;
    info->firstFreeCluster = current;
    PTR_TO_VALUE(32, FAT + last) = FAT32_CLSUTER_END_OF_CHAIN;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX;
}

void fat32_freeClusterChain(FAT32info* info, Index32 clusterChainFirst) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(info, clusterChainFirst) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = info->FAT;
    Index32 current = info->firstFreeCluster, last = INVALID_INDEX;
    while (current != FAT32_CLSUTER_END_OF_CHAIN) {
        last = current;
        current = PTR_TO_VALUE(32, FAT + current);
    }

    PTR_TO_VALUE(32, FAT + last) = info->firstFreeCluster;
    info->firstFreeCluster = clusterChainFirst;
}

Index32 fat32_cutClusterChain(FAT32info* info, Index32 cluster) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(info, cluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = info->FAT;
    Index32 ret = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = FAT32_CLSUTER_END_OF_CHAIN;

    return ret;
}

void fat32_insertClusterChain(FAT32info* info, Index32 cluster, Index32 clusterChainToInsert) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(info, cluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);
    DEBUG_ASSERT_SILENT(fat32_getClusterType(info, clusterChainToInsert) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = info->FAT;
    Index32 current = clusterChainToInsert;
    while (PTR_TO_VALUE(32, FAT + current) != FAT32_CLSUTER_END_OF_CHAIN) {
        current = PTR_TO_VALUE(32, FAT + current);
    }

    PTR_TO_VALUE(32, FAT + current) = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = clusterChainToInsert;
}