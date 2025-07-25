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
#include<error.h>

static fsNode* __fat32_fsCore_getFSnode(FScore* fsCore, ID vnodeID);

static vNode* __fat32_fsCore_openVnode(FScore* fsCore, ID vnodeID);

static vNode* __fat32_fsCore_openRootVnode(FScore* fsCore);

static void __fat32_fsCore_closeVnode(FScore* fsCore, vNode* vnode);

static void __fat32_fsCore_sync(FScore* fsCore);

static fsEntry* __fat32_fsCore_openFSentry(FScore* fsCore, vNode* vnode, FCNTLopenFlags flags);

static ConstCstring __fat32_name = "FAT32";
static FScoreOperations __fat32_fsCoreOperations = {
    .getFSnode      = __fat32_fsCore_getFSnode,
    .openVnode      = __fat32_fsCore_openVnode,
    .openRootVnode  = __fat32_fsCore_openRootVnode,
    .closeVnode     = __fat32_fsCore_closeVnode,
    .sync           = __fat32_fsCore_sync,
    .openFSentry    = __fat32_fsCore_openFSentry,
    .closeFSentry   = fsCore_genericCloseFSentry,
    .mount          = fsCore_genericMount,
    .unmount        = fsCore_genericUnmount
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

    void* BPBbuffer = NULL;

    Device* device = &blockDevice->device;
    BPBbuffer = mm_allocate(POWER_2(device->granularity));
    if (BPBbuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    blockDevice_readBlocks(blockDevice, 0, BPBbuffer, 1);
    ERROR_GOTO_IF_ERROR(0);

    FAT32BPB* BPB = (FAT32BPB*)BPBbuffer;
    Uint32 clusterNum = (device->capacity - BPB->reservedSectorNum - BPB->FATnum * BPB->sectorPerFAT) / BPB->sectorPerCluster - BPB->rootDirectoryClusterIndex;
    bool ret = BPB->bytePerSector == POWER_2(device->granularity) && BPB->signature == __FS_FAT32_BPB_SIGNATURE && memory_memcmp(BPB->systemIdentifier, "FAT32   ", 8) == 0 && clusterNum > __FS_FAT32_MINIMUM_CLUSTER_NUM;

    mm_free(BPBbuffer);

    return ret;
    ERROR_FINAL_BEGIN(0);
    if (BPBbuffer != NULL) {
        mm_free(BPBbuffer);
    }
    return false;
}

#define __FS_FAT32_FSCORE_HASH_BUCKET   16
#define __FS_FAT32_BATCH_ALLOCATE_SIZE  BATCH_ALLOCATE_SIZE((FAT32FScore, 1), (FAT32BPB, 1), (SinglyLinkedList, __FS_FAT32_FSCORE_HASH_BUCKET))

void fat32_open(FS* fs, BlockDevice* blockDevice) {
    void* batchAllocated = NULL, * buffer = NULL;
    batchAllocated = mm_allocate(__FS_FAT32_BATCH_ALLOCATE_SIZE);
    if (batchAllocated == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (FAT32FScore, fat32FScore, 1),
        (FAT32BPB, BPB, 1),
        (SinglyLinkedList, openedVnodeChains, __FS_FAT32_FSCORE_HASH_BUCKET)
    );

    Device* device = &blockDevice->device;
    buffer = mm_allocate(POWER_2(device->granularity));
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    blockDevice_readBlocks(blockDevice, 0, buffer, 1);
    ERROR_GOTO_IF_ERROR(0);

    memory_memcpy(BPB, buffer, sizeof(FAT32BPB));

    mm_free(buffer);
    buffer = NULL;

    if (POWER_2(device->granularity) != BPB->bytePerSector) {
        ERROR_THROW(ERROR_ID_DATA_ERROR, 0);
    }

    fat32FScore->FATrange               = RANGE_N(BPB->FATnum, BPB->reservedSectorNum, BPB->sectorPerFAT);

    Uint32 rootDirectorrySectorBegin    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT, rootDirectorrySectorLength = DIVIDE_ROUND_UP(sizeof(FAT32UnknownTypeEntry) * BPB->rootDirectoryEntryNum, BPB->bytePerSector);

    Uint32 dataBegin                    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT - 2 * BPB->sectorPerCluster, dataLength = device->capacity - dataBegin;
    fat32FScore->dataBlockRange         = RANGE(dataBegin, dataLength);

    Size clusterNum                     = DIVIDE_ROUND_DOWN(dataLength, BPB->sectorPerCluster);
    fat32FScore->clusterNum             = clusterNum;
    fat32FScore->BPB                    = BPB;

    Size FATsizeInByte                  = BPB->sectorPerFAT * POWER_2(device->granularity);

    Index32* FAT = mm_allocate(FATsizeInByte);
    if (FAT == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    blockDevice_readBlocks(blockDevice, fat32FScore->FATrange.begin, FAT, fat32FScore->FATrange.length);
    ERROR_GOTO_IF_ERROR(0);

    fat32FScore->FAT                    = FAT;

    Index32 firstFreeCluster = INVALID_INDEX32, last = INVALID_INDEX32;
    for (Index32 i = 0; i < clusterNum; ++i) {
        Index32 nextCluster = PTR_TO_VALUE(32, FAT + i);
        if (fat32_getClusterType(fat32FScore, nextCluster) != FAT32_CLUSTER_TYPE_FREE) {
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
    fat32FScore->firstFreeCluster   = firstFreeCluster;

    hashTable_initStruct(&fat32FScore->metadataTableVnodeID, FAT32_FSCORE_VNODE_TABLE_CHAIN_NUM, fat32FScore->metadataTableChainsVnodeID, hashTable_defaultHashFunc);
    hashTable_initStruct(&fat32FScore->metadataTableFirstCluster, FAT32_FSCORE_VNODE_TABLE_CHAIN_NUM, fat32FScore->metadataTableChainsFirstCluster, hashTable_defaultHashFunc);

    FScoreInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__fat32_fsCoreOperations,
        .openedVnodeBucket  = __FS_FAT32_FSCORE_HASH_BUCKET,
        .openedVnodeChains  = openedVnodeChains
    };

    fsCore_initStruct(&fat32FScore->fsCore, &args);

    fs->name                            = __fat32_name;
    fs->type                            = FS_TYPE_FAT32;
    fs->fsCore                          = &fat32FScore->fsCore;

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
    FScore* fsCore = fs->fsCore;
    FAT32FScore* fat32FScore = HOST_POINTER(fsCore, FAT32FScore, fsCore);

    fsCore_rawSync(fsCore);
    ERROR_GOTO_IF_ERROR(0);

    BlockDevice* fsCoreBlockDevice = fsCore->blockDevice;
    Device* fsCoreDevice = &fsCoreBlockDevice->device;
    Size FATsizeInByte = fat32FScore->FATrange.length * POWER_2(fsCoreDevice->granularity);
    memory_memset(fat32FScore->FAT, 0, FATsizeInByte);
    mm_free(fat32FScore->FAT);

    void* batchAllocated = fat32FScore; //TODO: Ugly code
    memory_memset(batchAllocated, 0, __FS_FAT32_BATCH_ALLOCATE_SIZE);
    mm_free(batchAllocated);

    return;
    ERROR_FINAL_BEGIN(0);
}

void fat32FScore_registerMetadata(FAT32FScore* fsCore, DirectoryEntry* entry, fsNode* belongTo, Index64 firstCluster, vNodeAttribute* vnodeAttribute) {
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

    hashTable_insert(&fsCore->metadataTableVnodeID, entry->vnodeID, &metadata->hashNodeVnodeID);
    hashTable_insert(&fsCore->metadataTableFirstCluster, firstCluster, &metadata->hashNodeFirstCluster);
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

void fat32FScore_unregisterMetadata(FAT32FScore* fsCore, Index64 firstCluster) {
    HashChainNode* deleted = hashTable_delete(&fsCore->metadataTableFirstCluster, firstCluster);
    if (deleted == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32NodeMetadata* metadata = HOST_POINTER(deleted, FAT32NodeMetadata, hashNodeFirstCluster);
    deleted = hashTable_delete(&fsCore->metadataTableVnodeID, FAT32_NODE_METADATA_GET_VNODE_ID(metadata));
    if (deleted == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    mm_free(metadata);

    return;
    ERROR_FINAL_BEGIN(0);
}

FAT32NodeMetadata* fat32FScore_getMetadataFromVnodeID(FAT32FScore* fsCore, ID vnodeID) {
    HashChainNode* found = hashTable_find(&fsCore->metadataTableVnodeID, vnodeID);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    return HOST_POINTER(found, FAT32NodeMetadata, hashNodeVnodeID);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

FAT32NodeMetadata* fat32FScore_getMetadataFromFirstCluster(FAT32FScore* fsCore, Index64 firstCluster) {
    HashChainNode* found = hashTable_find(&fsCore->metadataTableFirstCluster, firstCluster);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    return HOST_POINTER(found, FAT32NodeMetadata, hashNodeFirstCluster);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

Index32 fat32FScore_createFirstCluster(FAT32FScore* fsCore) {
    void* clusterBuffer = NULL;
    Index32 firstCluster = fat32_allocateClusterChain(fsCore, 1);   //First cluster of new file/directory must be all 0
    if (firstCluster == INVALID_INDEX32) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32BPB* BPB = fsCore->BPB;
    BlockDevice* targetBlockDevice = fsCore->fsCore.blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    Size clusterSize = BPB->sectorPerCluster * POWER_2(targetDevice->granularity);

    clusterBuffer = mm_allocate(clusterSize);
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    memory_memset(clusterBuffer, 0, clusterSize);
    blockDevice_writeBlocks(targetBlockDevice, fsCore->dataBlockRange.begin + (Index64)firstCluster * BPB->sectorPerCluster, clusterBuffer, BPB->sectorPerCluster);
    ERROR_GOTO_IF_ERROR(0);

    return firstCluster;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX32;
}

static fsNode* __fat32_fsCore_getFSnode(FScore* fsCore, ID vnodeID) {
    fsNode* node = NULL;

    FAT32FScore* fat32FScore = HOST_POINTER(fsCore, FAT32FScore, fsCore);
    FAT32NodeMetadata* metadata = fat32FScore_getMetadataFromVnodeID(fat32FScore, vnodeID);
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(vnodeID == FAT32_NODE_METADATA_GET_VNODE_ID(metadata));
    
    if (metadata->node != NULL) {
        fsNode_refer(metadata->node);
        return metadata->node;
    }

    node = fsNode_create(metadata->name.data, metadata->type, metadata->belongTo, vnodeID);
    if (node == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fsNode_refer(node);
    metadata->node = node;

    return node;
    ERROR_FINAL_BEGIN(0);
    if (node != NULL) {
        fsNode_release(node);
    }
    
    return NULL;
}

static vNode* __fat32_fsCore_openVnode(FScore* fsCore, ID vnodeID) {
    FAT32Vnode* fat32Vnode = NULL;

    fat32Vnode = mm_allocate(sizeof(FAT32Vnode));
    if (fat32Vnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32FScore* fat32FScore = HOST_POINTER(fsCore, FAT32FScore, fsCore);
    FAT32NodeMetadata* metadata = fat32FScore_getMetadataFromVnodeID(fat32FScore, vnodeID);
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32BPB* BPB               = fat32FScore->BPB;
    Device* fsCoreDevice        = &fsCore->blockDevice->device;
    fat32Vnode->firstCluster    = FAT32_NODE_METADATA_GET_FIRST_CLUSTER(metadata);
    fat32Vnode->isTouched       = metadata->isTouched;

    vNode* vnode = &fat32Vnode->vnode;
    vnode->sizeInByte       = metadata->size;
    vnode->sizeInBlock      = fat32_getClusterChainLength(fat32FScore, fat32Vnode->firstCluster) * BPB->sectorPerCluster;
    BlockDevice* fsCoreBlockDevice = fsCore->blockDevice;
    DEBUG_ASSERT_SILENT(vnode->sizeInByte <= vnode->sizeInBlock * POWER_2(fsCoreBlockDevice->device.granularity));
    
    vnode->signature        = VNODE_SIGNATURE;
    vnode->vnodeID          = vnodeID;
    vnode->fsCore           = fsCore;
    vnode->operations       = fat32_vNode_getOperations();
    
    REF_COUNTER_INIT(vnode->refCounter, 0);
    hashChainNode_initStruct(&vnode->openedNode);

    vnode->fsNode = fsCore_getFSnode(fsCore, vnodeID);
    if (vnode->fsNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    memory_memcpy(&vnode->attribute, &metadata->vnodeAttribute, sizeof(vNodeAttribute));

    vnode->deviceID         = INVALID_ID;
    vnode->lock             = SPINLOCK_UNLOCKED;
    
    if (vnode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY && !fat32Vnode->isTouched) {
        DEBUG_ASSERT_SILENT(vnode->sizeInByte == 0);
        vnode->sizeInByte   = fat32_vNode_touchDirectory(vnode);
        ERROR_GOTO_IF_ERROR(0);
        fat32Vnode->isTouched = metadata->isTouched = true;
    }
    
    vnode->fsNode->isVnodeActive = true;
    
    return vnode;
    ERROR_FINAL_BEGIN(0);
    if (fat32Vnode != NULL) {
        mm_free(fat32Vnode);
    }
    
    return NULL;
}

static vNode* __fat32_fsCore_openRootVnode(FScore* fsCore) {
    FAT32Vnode* fat32Vnode = NULL;
    
    ID vnodeID = fsCore_allocateVnodeID(fsCore);
    fsNode* rootNode = fsNode_create("", FS_ENTRY_TYPE_DIRECTORY, NULL, vnodeID);

    fat32Vnode = mm_allocate(sizeof(FAT32Vnode));
    if (fat32Vnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    vNode* vnode = &fat32Vnode->vnode;
    FAT32FScore* fat32FScore = HOST_POINTER(fsCore, FAT32FScore, fsCore);
    FAT32BPB* BPB = fat32FScore->BPB;

    fat32Vnode->firstCluster    = BPB->rootDirectoryClusterIndex;
    fat32Vnode->isTouched       = false;
    BlockDevice* fsCoreBlockDevice = fsCore->blockDevice;
    Size blockSize = POWER_2(fsCoreBlockDevice->device.granularity);
    vnode->sizeInBlock          = fat32_getClusterChainLength(fat32FScore, BPB->rootDirectoryClusterIndex) * BPB->sectorPerCluster;

    vnode->signature            = VNODE_SIGNATURE;
    vnode->vnodeID              = vnodeID;
    vnode->fsCore           = fsCore;
    vnode->operations           = fat32_vNode_getOperations();
    
    REF_COUNTER_INIT(vnode->refCounter, 0);
    hashChainNode_initStruct(&vnode->openedNode);
    vnode->fsNode = rootNode;
    fsNode_refer(vnode->fsNode);
    
    vnode->attribute = (vNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };
    
    vnode->deviceID             = INVALID_ID;
    vnode->lock                 = SPINLOCK_UNLOCKED;
    REF_COUNTER_REFER(vnode->refCounter);
    
    rootNode->isVnodeActive = true; //TODO: Ugly code
    
    vnode->sizeInByte           = fat32_vNode_touchDirectory(vnode);
    ERROR_GOTO_IF_ERROR(0);

    DEBUG_ASSERT_SILENT(vnode->sizeInByte <= vnode->sizeInBlock * blockSize);

    return vnode;
    ERROR_FINAL_BEGIN(0);
    if (fat32Vnode != NULL) {
        mm_free(fat32Vnode);
    }

    return NULL;
}

static void __fat32_fsCore_closeVnode(FScore* fsCore, vNode* vnode) {
    if (REF_COUNTER_DEREFER(vnode->refCounter) != 0) {
        return;
    }
    
    FAT32Vnode* fat32Vnode = HOST_POINTER(vnode, FAT32Vnode, vnode);
    fsNode_release(vnode->fsNode);

    mm_free(fat32Vnode);
}

static void __fat32_fsCore_sync(FScore* fsCore) {
    Index32* FATcopy = NULL;

    FAT32FScore* fat32FScore = HOST_POINTER(fsCore, FAT32FScore, fsCore);
    
    FAT32BPB* BPB = fat32FScore->BPB;
    BlockDevice* fsCoreBlockDevice = fsCore->blockDevice;
    Device* fsCoreDevice = &fsCoreBlockDevice->device;

    RangeN* fatRange = &fat32FScore->FATrange;
    Size FATsizeInByte = fatRange->length * POWER_2(fsCoreDevice->granularity);
    FATcopy = mm_allocate(FATsizeInByte);
    if (FATcopy == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memcpy(FATcopy, fat32FScore->FAT, FATsizeInByte);

    for (int i = fat32FScore->firstFreeCluster, next; i != FAT32_CLSUTER_END_OF_CHAIN; i = next) {
        next = PTR_TO_VALUE(32, FATcopy + i);
        PTR_TO_VALUE(32, FATcopy + i) = 0;
    }

    for (int i = 0; i < fatRange->n; ++i) { //TODO: FAT is only saved here, maybe we can make it save at anytime
        blockDevice_writeBlocks(fsCoreBlockDevice, fatRange->begin + i * fatRange->length, FATcopy, fatRange->length);
        ERROR_GOTO_IF_ERROR(0);
    }

    mm_free(FATcopy);
    FATcopy = NULL;

    blockDevice_flush(fsCoreBlockDevice);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
    if (FATcopy != NULL) {
        mm_free(FATcopy);
    }
    return;
}

static fsEntry* __fat32_fsCore_openFSentry(FScore* fsCore, vNode* vnode, FCNTLopenFlags flags) {
    fsEntry* ret = fsCore_genericOpenFSentry(fsCore, vnode, flags);
    ERROR_GOTO_IF_ERROR(0);

    ret->operations = &_fat32_fsEntryOperations;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}