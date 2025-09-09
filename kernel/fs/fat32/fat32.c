#include<fs/fat32/fat32.h>

#include<devices/blockDevice.h>
#include<devices/device.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/directoryEntry.h>
#include<fs/fat32/vnode.h>
#include<fs/directoryEntry.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/fscore.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<multitask/locks/spinlock.h>
#include<structs/hashTable.h>
#include<structs/string.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>

static vNode* __fat32_fscore_openVnode(FScore* fscore, fsNode* node);

static void __fat32_fscore_closeVnode(FScore* fscore, vNode* vnode);

static void __fat32_fscore_sync(FScore* fscore);

static fsEntry* __fat32_fscore_openFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags);

static ConstCstring __fat32_name = "FAT32";
static FScoreOperations __fat32_fscoreOperations = {
    .openVnode      = __fat32_fscore_openVnode,
    .closeVnode     = __fat32_fscore_closeVnode,
    .sync           = __fat32_fscore_sync,
    .openFSentry    = __fat32_fscore_openFSentry,
    .closeFSentry   = fscore_genericCloseFSentry,
    .mount          = fscore_genericMount,
    .unmount        = fscore_genericUnmount
};

static fsEntryOperations _fat32_fsEntryOperations = {
    .seek           = fsEntry_genericSeek,
    .read           = fsEntry_genericRead,
    .write          = fsEntry_genericWrite
};

void fat32_init() {
    fat32_vNode_init();
}

#define __FS_FAT32_BPB_SIGNATURE        0x29
#define __FS_FAT32_MINIMUM_CLUSTER_NUM  65525

bool fat32_checkType(BlockDevice* blockDevice) {
    if (blockDevice == NULL) {
        return false;
    }

    Device* device = &blockDevice->device;
    Uint8 BPBbuffer[algorithms_umax64(POWER_2(device->granularity), sizeof(FAT32BPB))];

    blockDevice_readBlocks(blockDevice, 0, (void*)BPBbuffer, 1);
    ERROR_GOTO_IF_ERROR(0);

    FAT32BPB* BPB = (FAT32BPB*)BPBbuffer;
    Uint32 clusterNum = (device->capacity - BPB->reservedSectorNum - BPB->FATnum * BPB->sectorPerFAT) / BPB->sectorPerCluster - BPB->rootDirectoryClusterIndex;
    bool ret = BPB->bytePerSector == POWER_2(device->granularity) && BPB->signature == __FS_FAT32_BPB_SIGNATURE && memory_memcmp(BPB->systemIdentifier, "FAT32   ", 8) == 0 && clusterNum > __FS_FAT32_MINIMUM_CLUSTER_NUM;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return false;
}

// #define __FS_FAT32_FSCORE_HASH_BUCKET   16
// #define __FS_FAT32_BATCH_ALLOCATE_SIZE  BATCH_ALLOCATE_SIZE((FAT32fscore, 1), (FAT32BPB, 1), (SinglyLinkedList, __FS_FAT32_FSCORE_HASH_BUCKET))
#define __FS_FAT32_BATCH_ALLOCATE_SIZE  BATCH_ALLOCATE_SIZE((FAT32fscore, 1), (FAT32BPB, 1))

void fat32_open(FS* fs, BlockDevice* blockDevice) {
    void* batchAllocated = NULL, * buffer = NULL;
    batchAllocated = mm_allocate(__FS_FAT32_BATCH_ALLOCATE_SIZE);
    if (batchAllocated == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (FAT32fscore, fat32fscore, 1),
        (FAT32BPB, BPB, 1),
        // (SinglyLinkedList, openedVnodeChains, __FS_FAT32_FSCORE_HASH_BUCKET)
    );

    Device* device = &blockDevice->device;
    buffer = mm_allocate(POWER_2(device->granularity));
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    blockDevice_readBlocks(blockDevice, 0, buffer, 1);  //TODO: One block?
    ERROR_GOTO_IF_ERROR(0);

    memory_memcpy(BPB, buffer, sizeof(FAT32BPB));

    mm_free(buffer);
    buffer = NULL;

    if (POWER_2(device->granularity) != BPB->bytePerSector) {
        ERROR_THROW(ERROR_ID_DATA_ERROR, 0);
    }

    fat32fscore->FATrange               = RANGE_N(BPB->FATnum, BPB->reservedSectorNum, BPB->sectorPerFAT);

    Uint32 rootDirectorrySectorBegin    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT, rootDirectorrySectorLength = DIVIDE_ROUND_UP(sizeof(FAT32UnknownTypeEntry) * BPB->rootDirectoryEntryNum, BPB->bytePerSector);

    Uint32 dataBegin                    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT - 2 * BPB->sectorPerCluster, dataLength = device->capacity - dataBegin;
    fat32fscore->dataBlockRange         = RANGE(dataBegin, dataLength);

    Size clusterNum                     = DIVIDE_ROUND_DOWN(dataLength, BPB->sectorPerCluster);
    fat32fscore->clusterNum             = clusterNum;
    fat32fscore->BPB                    = BPB;

    Size FATsizeInByte                  = BPB->sectorPerFAT * POWER_2(device->granularity);

    Index32* FAT = mm_allocate(FATsizeInByte);
    if (FAT == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    blockDevice_readBlocks(blockDevice, fat32fscore->FATrange.begin, FAT, fat32fscore->FATrange.length);
    ERROR_GOTO_IF_ERROR(0);

    fat32fscore->FAT                    = FAT;

    Index32 firstFreeCluster = INVALID_INDEX32, last = INVALID_INDEX32;
    for (Index32 i = 0; i < clusterNum; ++i) {
        Index32 nextCluster = PTR_TO_VALUE(32, FAT + i);
        if (fat32_getClusterType(fat32fscore, nextCluster) != FAT32_CLUSTER_TYPE_FREE) {
            continue;
        }

        if (firstFreeCluster == INVALID_INDEX32) {
            firstFreeCluster = i;
        } else {
            PTR_TO_VALUE(32, FAT + last) = i;
        }

        last = i;
    }
    PTR_TO_VALUE(32, FAT + last) = FAT32_CLSUTER_END_OF_CHAIN;
    fat32fscore->firstFreeCluster   = firstFreeCluster;

    // hashTable_initStruct(&fat32fscore->metadataTableVnodeID, FAT32_FSCORE_VNODE_TABLE_CHAIN_NUM, fat32fscore->metadataTableChainsVnodeID, hashTable_defaultHashFunc);
    hashTable_initStruct(&fat32fscore->metadataTableFirstCluster, FAT32_FSCORE_VNODE_TABLE_CHAIN_NUM, fat32fscore->metadataTableChainsFirstCluster, hashTable_defaultHashFunc);

    DirectoryEntry rootDirEntry = {
        .name = "",
        .type = FS_ENTRY_TYPE_DIRECTORY,
        .vnodeID = 0,
        .size = 0,
        .mode = 0,
        .deviceID = 0
    };

    vNodeAttribute rootDirAttribute = {
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };

    fat32FScore_registerMetadata(fat32fscore, &rootDirEntry, NULL, BPB->rootDirectoryClusterIndex, &rootDirAttribute);
    
    FScoreInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__fat32_fscoreOperations,
        .rootFSnodePosition = BPB->rootDirectoryClusterIndex
        // .openedVnodeBucket  = __FS_FAT32_FSCORE_HASH_BUCKET,
        // .openedVnodeChains  = openedVnodeChains
    };

    fscore_initStruct(&fat32fscore->fscore, &args);

    fs->name                            = __fat32_name;
    fs->type                            = FS_TYPE_FAT32;
    fs->fscore                          = &fat32fscore->fscore;

    return;
    ERROR_FINAL_BEGIN(0);
    if (FAT != NULL) {
        mm_free(FAT);
    }

    if (buffer != NULL) {
        mm_free(buffer);
    }
    
    if (batchAllocated != NULL) {
        mm_free(batchAllocated);
    }
}

void fat32_close(FS* fs) {
    FScore* fscore = fs->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);

    fscore_rawSync(fscore);
    ERROR_GOTO_IF_ERROR(0);

    BlockDevice* fscoreBlockDevice = fscore->blockDevice;
    Device* fscoreDevice = &fscoreBlockDevice->device;
    Size FATsizeInByte = fat32fscore->FATrange.length * POWER_2(fscoreDevice->granularity);
    memory_memset(fat32fscore->FAT, 0, FATsizeInByte);
    mm_free(fat32fscore->FAT);

    void* batchAllocated = fat32fscore; //TODO: Ugly code
    memory_memset(batchAllocated, 0, __FS_FAT32_BATCH_ALLOCATE_SIZE);
    mm_free(batchAllocated);

    return;
    ERROR_FINAL_BEGIN(0);
}

void fat32FScore_registerMetadata(FAT32fscore* fscore, DirectoryEntry* entry, fsNode* belongTo, Index64 firstCluster, vNodeAttribute* vnodeAttribute) {
    FAT32NodeMetadata* metadata = NULL;
    metadata = mm_allocate(sizeof(FAT32NodeMetadata));
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    string_initStructStr(&metadata->name, entry->name);
    ERROR_GOTO_IF_ERROR(0);
    metadata->type = entry->type;
    metadata->node = NULL;
    metadata->belongTo = belongTo;
    memory_memcpy(&metadata->vnodeAttribute, vnodeAttribute, sizeof(vNodeAttribute));
    metadata->size = entry->size;
    metadata->isTouched = false;

    // hashTable_insert(&fscore->metadataTableVnodeID, entry->vnodeID, &metadata->hashNodeVnodeID);
    hashTable_insert(&fscore->metadataTableFirstCluster, (Object)firstCluster, &metadata->hashNodeFirstCluster);
    ERROR_CHECKPOINT({ 
            ERROR_GOTO(0);
        },
        (ERROR_ID_ALREADY_EXIST, {
            ERROR_CLEAR();
            mm_free(metadata);
        })
    );

    return;
    ERROR_FINAL_BEGIN(0);
    if (metadata != NULL) {
        mm_free(metadata);
    }
}

void fat32FScore_unregisterMetadata(FAT32fscore* fscore, Index64 firstCluster) {
    HashChainNode* deleted = hashTable_delete(&fscore->metadataTableFirstCluster, firstCluster);
    if (deleted == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32NodeMetadata* metadata = HOST_POINTER(deleted, FAT32NodeMetadata, hashNodeFirstCluster);
    // deleted = hashTable_delete(&fscore->metadataTableVnodeID, FAT32_NODE_METADATA_GET_VNODE_ID(metadata));
    // if (deleted == NULL) {
    //     ERROR_ASSERT_ANY();
    //     ERROR_GOTO(0);
    // }

    mm_free(metadata);

    return;
    ERROR_FINAL_BEGIN(0);
}

// FAT32NodeMetadata* fat32FScore_getMetadataFromVnodeID(FAT32fscore* fscore, ID vnodeID) {
//     HashChainNode* found = hashTable_find(&fscore->metadataTableVnodeID, vnodeID);
//     if (found == NULL) {
//         ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
//     }

//     return HOST_POINTER(found, FAT32NodeMetadata, hashNodeVnodeID);
//     ERROR_FINAL_BEGIN(0);
//     return NULL;
// }

FAT32NodeMetadata* fat32FScore_getMetadataFromFirstCluster(FAT32fscore* fscore, Index64 firstCluster) {
    HashChainNode* found = hashTable_find(&fscore->metadataTableFirstCluster, firstCluster);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    return HOST_POINTER(found, FAT32NodeMetadata, hashNodeFirstCluster);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

Index32 fat32FScore_createFirstCluster(FAT32fscore* fscore) {
    void* clusterBuffer = NULL;
    Index32 firstCluster = fat32_allocateClusterChain(fscore, 1);   //First cluster of new file/directory must be all 0
    if (firstCluster == INVALID_INDEX32) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32BPB* BPB = fscore->BPB;
    BlockDevice* targetBlockDevice = fscore->fscore.blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    Size clusterSize = BPB->sectorPerCluster * POWER_2(targetDevice->granularity);

    clusterBuffer = mm_allocate(clusterSize);
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    memory_memset(clusterBuffer, 0, clusterSize);
    blockDevice_writeBlocks(targetBlockDevice, fscore->dataBlockRange.begin + (Index64)firstCluster * BPB->sectorPerCluster, clusterBuffer, BPB->sectorPerCluster);
    ERROR_GOTO_IF_ERROR(0);

    return firstCluster;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX32;
}

static vNode* __fat32_fscore_openVnode(FScore* fscore, fsNode* node) {
    FAT32vnode* fat32vnode = NULL;

    fat32vnode = mm_allocate(sizeof(FAT32vnode));
    if (fat32vnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32fscore* fat32fscore    = HOST_POINTER(fscore, FAT32fscore, fscore);
    FAT32BPB* BPB               = fat32fscore->BPB;
    FAT32NodeMetadata* metadata = fat32FScore_getMetadataFromFirstCluster(fat32fscore, node->physicalPosition);
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Device* fscoreDevice        = &fscore->blockDevice->device;
    fat32vnode->firstCluster    = node->physicalPosition;
    fat32vnode->isTouched       = metadata->isTouched;

    vNode* vnode = &fat32vnode->vnode;
    vnode->sizeInByte       = metadata->size;
    vnode->sizeInBlock      = fat32_getClusterChainLength(fat32fscore, fat32vnode->firstCluster) * BPB->sectorPerCluster;
    BlockDevice* fscoreBlockDevice = fscore->blockDevice;
    DEBUG_ASSERT_SILENT(vnode->sizeInByte <= vnode->sizeInBlock * POWER_2(fscoreBlockDevice->device.granularity));
    
    vnode->signature        = VNODE_SIGNATURE;
    vnode->vnodeID          = (ID)node->physicalPosition;   //TODO: Ugly code
    vnode->fscore           = fscore;
    vnode->operations       = fat32_vNode_getOperations();
    
    REF_COUNTER_INIT(vnode->refCounter, 0);
    hashChainNode_initStruct(&vnode->openedNode);

    vnode->fsNode = node;
    
    memory_memcpy(&vnode->attribute, &metadata->vnodeAttribute, sizeof(vNodeAttribute));

    vnode->deviceID         = INVALID_ID;
    vnode->lock             = SPINLOCK_UNLOCKED;
    
    if (vnode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY && !fat32vnode->isTouched) {
        DEBUG_ASSERT_SILENT(vnode->sizeInByte == 0);
        vnode->sizeInByte   = fat32_vNode_touchDirectory(vnode);
        ERROR_GOTO_IF_ERROR(0);
        fat32vnode->isTouched = metadata->isTouched = true;
    }
    
    return vnode;
    ERROR_FINAL_BEGIN(0);
    if (fat32vnode != NULL) {
        mm_free(fat32vnode);
    }
    
    return NULL;
}

static void __fat32_fscore_closeVnode(FScore* fscore, vNode* vnode) {
    if (REF_COUNTER_DEREFER(vnode->refCounter) != 0) {
        return;
    }
    
    FAT32vnode* fat32vnode = HOST_POINTER(vnode, FAT32vnode, vnode);

    mm_free(fat32vnode);
}

static void __fat32_fscore_sync(FScore* fscore) {
    Index32* FATcopy = NULL;

    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    
    FAT32BPB* BPB = fat32fscore->BPB;
    BlockDevice* fscoreBlockDevice = fscore->blockDevice;
    Device* fscoreDevice = &fscoreBlockDevice->device;

    RangeN* fatRange = &fat32fscore->FATrange;
    Size FATsizeInByte = fatRange->length * POWER_2(fscoreDevice->granularity);
    FATcopy = mm_allocate(FATsizeInByte);
    if (FATcopy == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memcpy(FATcopy, fat32fscore->FAT, FATsizeInByte);

    for (int i = fat32fscore->firstFreeCluster, next; i != FAT32_CLSUTER_END_OF_CHAIN; i = next) {
        next = PTR_TO_VALUE(32, FATcopy + i);
        PTR_TO_VALUE(32, FATcopy + i) = 0;
    }

    for (int i = 0; i < fatRange->n; ++i) { //TODO: FAT is only saved here, maybe we can make it save at anytime
        blockDevice_writeBlocks(fscoreBlockDevice, fatRange->begin + i * fatRange->length, FATcopy, fatRange->length);
        ERROR_GOTO_IF_ERROR(0);
    }

    mm_free(FATcopy);
    FATcopy = NULL;

    blockDevice_flush(fscoreBlockDevice);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
    if (FATcopy != NULL) {
        mm_free(FATcopy);
    }
    return;
}

static fsEntry* __fat32_fscore_openFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags) {
    fsEntry* ret = fscore_genericOpenFSentry(fscore, vnode, flags);
    ERROR_GOTO_IF_ERROR(0);

    ret->operations = &_fat32_fsEntryOperations;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}