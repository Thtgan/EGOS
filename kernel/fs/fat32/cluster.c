#include<fs/fat32/cluster.h>

#include<fs/fat32/fat32.h>
#include<kit/types.h>
#include<kit/util.h>
#include<debug.h>
#include<error.h>

//TODO: These codes seems wrong
FAT32ClusterType fat32_getClusterType(FAT32fscore* fscore, Index32 physicalClusterIndex) {
    if (physicalClusterIndex == 0x00000000) {
        return FAT32_CLUSTER_TYPE_FREE;
    }

    if (0x00000002 <= physicalClusterIndex && physicalClusterIndex < fscore->clusterNum) {
        return FAT32_CLUSTER_TYPE_ALLOCATERD;
    }

    if (fscore->clusterNum <= physicalClusterIndex && physicalClusterIndex < 0x0FFFFFF7) {
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

Index32 fat32_getCluster(FAT32fscore* fscore, Index32 firstCluster, Index32 index) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(fscore, firstCluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = fscore->FAT;
    Index32 currentCluster = firstCluster;
    for (int i = 0; i < index; ++i) {
        if (currentCluster == FAT32_CLSUTER_END_OF_CHAIN) {
            ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
        }
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
    }

    return currentCluster;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX32;
}

Index32 fat32_stepCluster(FAT32fscore* fscore, Index32 firstCluster, Size n, Size* continousRet) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(fscore, firstCluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);
    Size continousLength = 1;
    Index32* FAT = fscore->FAT;
    Index32 currentCluster = firstCluster;
    while (continousLength < n && FAT[currentCluster] == currentCluster + 1) {
        ++currentCluster;
        ++continousLength;
    }

    *continousRet = continousLength;

    Index32 nextCluster = FAT[currentCluster];
    return fat32_getClusterType(fscore, nextCluster) == FAT32_CLUSTER_TYPE_ALLOCATERD ? nextCluster : INVALID_INDEX32;
}

Size fat32_getClusterChainLength(FAT32fscore* fscore, Index32 firstCluster) {
    if (fat32_getClusterType(fscore, firstCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return 0;
    }

    Size ret = 1;
    Index32* FAT = fscore->FAT;

    Index32 currentCluster = firstCluster;
    while (true) {
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
        if (fat32_getClusterType(fscore, currentCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            break;
        }
        ++ret;
    }

    return ret;
}

Index32 fat32_allocateClusterChain(FAT32fscore* fscore, Size length) {
    Index32* FAT = fscore->FAT;
    Index32 currentCluster = fscore->firstFreeCluster, last = INVALID_INDEX32;
    for (int i = 0; i < length; ++i) {
        if (currentCluster == FAT32_CLSUTER_END_OF_CHAIN) {
            ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
        }

        last = currentCluster;
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
    }

    Index32 ret = fscore->firstFreeCluster;
    fscore->firstFreeCluster = currentCluster;
    PTR_TO_VALUE(32, FAT + last) = FAT32_CLSUTER_END_OF_CHAIN;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX32;
}

void fat32_freeClusterChain(FAT32fscore* fscore, Index32 clusterChainFirst) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(fscore, clusterChainFirst) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = fscore->FAT;
    Index32 currentCluster = fscore->firstFreeCluster, last = INVALID_INDEX32;
    while (currentCluster != FAT32_CLSUTER_END_OF_CHAIN) {
        last = currentCluster;
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
    }
    DEBUG_ASSERT_SILENT(last != INVALID_INDEX32);

    PTR_TO_VALUE(32, FAT + last) = fscore->firstFreeCluster;
    fscore->firstFreeCluster = clusterChainFirst;
}

Index32 fat32_cutClusterChain(FAT32fscore* fscore, Index32 cluster) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(fscore, cluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = fscore->FAT;
    Index32 ret = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = FAT32_CLSUTER_END_OF_CHAIN;

    return ret;
}

void fat32_insertClusterChain(FAT32fscore* fscore, Index32 cluster, Index32 clusterChainToInsert) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(fscore, cluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);
    DEBUG_ASSERT_SILENT(fat32_getClusterType(fscore, clusterChainToInsert) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = fscore->FAT;
    Index32 currentCluster = clusterChainToInsert;
    while (PTR_TO_VALUE(32, FAT + currentCluster) != FAT32_CLSUTER_END_OF_CHAIN) {
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
    }

    PTR_TO_VALUE(32, FAT + currentCluster) = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = clusterChainToInsert;
}