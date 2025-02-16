#include<fs/fat32/cluster.h>

#include<fs/fat32/fat32.h>
#include<kit/types.h>
#include<kit/util.h>
#include<debug.h>
#include<error.h>

//TODO: These codes seems wrong
FAT32ClusterType fat32_getClusterType(FAT32SuperBlock* superBlock, Index32 physicalClusterIndex) {
    if (physicalClusterIndex == 0x00000000) {
        return FAT32_CLUSTER_TYPE_FREE;
    }

    if (0x00000002 <= physicalClusterIndex && physicalClusterIndex < superBlock->clusterNum) {
        return FAT32_CLUSTER_TYPE_ALLOCATERD;
    }

    if (superBlock->clusterNum <= physicalClusterIndex && physicalClusterIndex < 0x0FFFFFF7) {
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

Index32 fat32_getCluster(FAT32SuperBlock* superBlock, Index32 firstCluster, Index32 index) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(superBlock, firstCluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = superBlock->FAT;
    Index32 currentCluster = firstCluster;
    for (int i = 0; i < index; ++i) {
        if (currentCluster == FAT32_CLSUTER_END_OF_CHAIN) {
            ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
        }
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
    }

    return currentCluster;
    ERROR_FINAL_BEGIN(0);
    return (Uint32)INVALID_INDEX;
}

Index32 fat32_stepCluster(FAT32SuperBlock* superBlock, Index32 firstCluster, Size n, Size* continousRet) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(superBlock, firstCluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);
    Size continousLength = 1;
    Index32* FAT = superBlock->FAT;
    Index32 currentCluster = firstCluster;
    while (continousLength < n && FAT[currentCluster] == currentCluster + 1) {
        ++currentCluster;
        ++continousLength;
    }

    *continousRet = continousLength;

    Index32 nextCluster = FAT[currentCluster];
    return fat32_getClusterType(superBlock, nextCluster) == FAT32_CLUSTER_TYPE_ALLOCATERD ? nextCluster : (Uint32)INVALID_INDEX;
}

Size fat32_getClusterChainLength(FAT32SuperBlock* superBlock, Index32 firstCluster) {
    if (fat32_getClusterType(superBlock, firstCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
        return 0;
    }

    Size ret = 1;
    Index32* FAT = superBlock->FAT;

    Index32 currentCluster = firstCluster;
    while (true) {
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
        if (fat32_getClusterType(superBlock, currentCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            break;
        }
        ++ret;
    }

    return ret;
}

Index32 fat32_allocateClusterChain(FAT32SuperBlock* superBlock, Size length) {
    Index32* FAT = superBlock->FAT;
    Index32 currentCluster = superBlock->firstFreeCluster, last = (Uint32)INVALID_INDEX;
    for (int i = 0; i < length; ++i) {
        if (currentCluster == FAT32_CLSUTER_END_OF_CHAIN) {
            ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
        }

        last = currentCluster;
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
    }

    Index32 ret = superBlock->firstFreeCluster;
    superBlock->firstFreeCluster = currentCluster;
    PTR_TO_VALUE(32, FAT + last) = FAT32_CLSUTER_END_OF_CHAIN;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return (Uint32)INVALID_INDEX;
}

void fat32_freeClusterChain(FAT32SuperBlock* superBlock, Index32 clusterChainFirst) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(superBlock, clusterChainFirst) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = superBlock->FAT;
    Index32 currentCluster = superBlock->firstFreeCluster, last = (Uint32)INVALID_INDEX;
    while (currentCluster != FAT32_CLSUTER_END_OF_CHAIN) {
        last = currentCluster;
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
    }

    PTR_TO_VALUE(32, FAT + last) = superBlock->firstFreeCluster;
    superBlock->firstFreeCluster = clusterChainFirst;
}

Index32 fat32_cutClusterChain(FAT32SuperBlock* superBlock, Index32 cluster) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(superBlock, cluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = superBlock->FAT;
    Index32 ret = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = FAT32_CLSUTER_END_OF_CHAIN;

    return ret;
}

void fat32_insertClusterChain(FAT32SuperBlock* superBlock, Index32 cluster, Index32 clusterChainToInsert) {
    DEBUG_ASSERT_SILENT(fat32_getClusterType(superBlock, cluster) == FAT32_CLUSTER_TYPE_ALLOCATERD);
    DEBUG_ASSERT_SILENT(fat32_getClusterType(superBlock, clusterChainToInsert) == FAT32_CLUSTER_TYPE_ALLOCATERD);

    Index32* FAT = superBlock->FAT;
    Index32 currentCluster = clusterChainToInsert;
    while (PTR_TO_VALUE(32, FAT + currentCluster) != FAT32_CLSUTER_END_OF_CHAIN) {
        currentCluster = PTR_TO_VALUE(32, FAT + currentCluster);
    }

    PTR_TO_VALUE(32, FAT + currentCluster) = PTR_TO_VALUE(32, FAT + cluster);
    PTR_TO_VALUE(32, FAT + cluster) = clusterChainToInsert;
}