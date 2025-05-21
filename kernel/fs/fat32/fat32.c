#include<fs/fat32/fat32.h>

#include<devices/blockDevice.h>
#include<devices/device.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/directoryEntry.h>
#include<fs/fat32/inode.h>
#include<fs/directoryEntry.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<multitask/locks/spinlock.h>
#include<structs/hashTable.h>
#include<structs/string.h>
#include<system/pageTable.h>
#include<error.h>

static fsNode* __fat32_superBlock_getFSnode(SuperBlock* superBlock, ID inodeID);

static iNode* __fat32_superBlock_openInode(SuperBlock* superBlock, ID inodeID);

static iNode* __fat32_superBlock_openRootInode(SuperBlock* superBlock);

static void __fat32_superBlock_closeInode(SuperBlock* superBlock, iNode* inode);

static void __fat32_superBlock_sync(SuperBlock* superBlock);

static fsEntry* __fat32_superBlock_openFSentry(SuperBlock* superBlock, iNode* inode, FCNTLopenFlags flags);

static ConstCstring __fat32_name = "FAT32";
static SuperBlockOperations __fat32_superBlockOperations = {
    .getFSnode      = __fat32_superBlock_getFSnode,
    .openInode      = __fat32_superBlock_openInode,
    .openRootInode  = __fat32_superBlock_openRootInode,
    .closeInode     = __fat32_superBlock_closeInode,
    .sync           = __fat32_superBlock_sync,
    .openFSentry    = __fat32_superBlock_openFSentry,
    .closeFSentry   = superBlock_genericCloseFSentry,
    .mount          = superBlock_genericMount,
    .unmount        = superBlock_genericUnmount
};

static fsEntryOperations _fat32_fsEntryOperations = {
    .seek           = fsEntry_genericSeek,
    .read           = fsEntry_genericRead,
    .write          = fsEntry_genericWrite
};

void fat32_init() {
    fat32_iNode_init();
}

#define __FS_FAT32_BPB_SIGNATURE        0x29
#define __FS_FAT32_MINIMUM_CLUSTER_NUM  65525

bool fat32_checkType(BlockDevice* blockDevice) {
    if (blockDevice == NULL) {
        return false;
    }

    void* BPBbuffer = NULL;

    Device* device = &blockDevice->device;
    BPBbuffer = memory_allocate(POWER_2(device->granularity));
    if (BPBbuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    blockDevice_readBlocks(blockDevice, 0, BPBbuffer, 1);
    ERROR_GOTO_IF_ERROR(0);

    FAT32BPB* BPB = (FAT32BPB*)BPBbuffer;
    Uint32 clusterNum = (device->capacity - BPB->reservedSectorNum - BPB->FATnum * BPB->sectorPerFAT) / BPB->sectorPerCluster - BPB->rootDirectoryClusterIndex;
    bool ret = BPB->bytePerSector == POWER_2(device->granularity) && BPB->signature == __FS_FAT32_BPB_SIGNATURE && memory_memcmp(BPB->systemIdentifier, "FAT32   ", 8) == 0 && clusterNum > __FS_FAT32_MINIMUM_CLUSTER_NUM;

    memory_free(BPBbuffer);

    return ret;
    ERROR_FINAL_BEGIN(0);
    if (BPBbuffer != NULL) {
        memory_free(BPBbuffer);
    }
    return false;
}

#define __FS_FAT32_SUPERBLOCK_HASH_BUCKET   16
#define __FS_FAT32_BATCH_ALLOCATE_SIZE      BATCH_ALLOCATE_SIZE((FAT32SuperBlock, 1), (FAT32BPB, 1), (SinglyLinkedList, __FS_FAT32_SUPERBLOCK_HASH_BUCKET))

void fat32_open(FS* fs, BlockDevice* blockDevice) {
    void* batchAllocated = NULL, * buffer = NULL;
    batchAllocated = memory_allocate(__FS_FAT32_BATCH_ALLOCATE_SIZE);
    if (batchAllocated == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (FAT32SuperBlock, fat32SuperBlock, 1),
        (FAT32BPB, BPB, 1),
        (SinglyLinkedList, openedInodeChains, __FS_FAT32_SUPERBLOCK_HASH_BUCKET)
    );

    Device* device = &blockDevice->device;
    buffer = memory_allocate(POWER_2(device->granularity));
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    blockDevice_readBlocks(blockDevice, 0, buffer, 1);
    ERROR_GOTO_IF_ERROR(0);

    memory_memcpy(BPB, buffer, sizeof(FAT32BPB));

    memory_free(buffer);
    buffer = NULL;

    if (POWER_2(device->granularity) != BPB->bytePerSector) {
        ERROR_THROW(ERROR_ID_DATA_ERROR, 0);
    }

    fat32SuperBlock->FATrange           = RANGE_N(BPB->FATnum, BPB->reservedSectorNum, BPB->sectorPerFAT);

    Uint32 rootDirectorrySectorBegin    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT, rootDirectorrySectorLength = DIVIDE_ROUND_UP(sizeof(FAT32UnknownTypeEntry) * BPB->rootDirectoryEntryNum, BPB->bytePerSector);

    Uint32 dataBegin                    = BPB->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT - 2 * BPB->sectorPerCluster, dataLength = device->capacity - dataBegin;
    fat32SuperBlock->dataBlockRange     = RANGE(dataBegin, dataLength);

    Size clusterNum                     = DIVIDE_ROUND_DOWN(dataLength, BPB->sectorPerCluster);
    fat32SuperBlock->clusterNum         = clusterNum;
    fat32SuperBlock->BPB                = BPB;

    Size FATsizeInByte                  = BPB->sectorPerFAT * POWER_2(device->granularity);

    Index32* FAT = memory_allocate(FATsizeInByte);
    if (FAT == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    blockDevice_readBlocks(blockDevice, fat32SuperBlock->FATrange.begin, FAT, fat32SuperBlock->FATrange.length);
    ERROR_GOTO_IF_ERROR(0);

    fat32SuperBlock->FAT                = FAT;

    Index32 firstFreeCluster = INVALID_INDEX, last = INVALID_INDEX;
    for (Index32 i = 0; i < clusterNum; ++i) {
        Index32 nextCluster = PTR_TO_VALUE(32, FAT + i);
        if (fat32_getClusterType(fat32SuperBlock, nextCluster) != FAT32_CLUSTER_TYPE_FREE) {
            continue;
        }

        if (firstFreeCluster == INVALID_INDEX) {
            firstFreeCluster = i;
        } else {
            PTR_TO_VALUE(32, FAT + last) = i;
        }

        last = i;
    }
    PTR_TO_VALUE(32, FAT + last) = FAT32_CLSUTER_END_OF_CHAIN;
    fat32SuperBlock->firstFreeCluster   = firstFreeCluster;

    hashTable_initStruct(&fat32SuperBlock->metadataTableInodeID, FAT32_SUPERBLOCK_INODE_TABLE_CHAIN_NUM, fat32SuperBlock->metadataTableChainsInodeID, hashTable_defaultHashFunc);
    hashTable_initStruct(&fat32SuperBlock->metadataTableFirstCluster, FAT32_SUPERBLOCK_INODE_TABLE_CHAIN_NUM, fat32SuperBlock->metadataTableChainsFirstCluster, hashTable_defaultHashFunc);

    SuperBlockInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__fat32_superBlockOperations,
        .openedInodeBucket  = __FS_FAT32_SUPERBLOCK_HASH_BUCKET,
        .openedInodeChains  = openedInodeChains
    };

    superBlock_initStruct(&fat32SuperBlock->superBlock, &args);

    fs->name                            = __fat32_name;
    fs->type                            = FS_TYPE_FAT32;
    fs->superBlock                      = &fat32SuperBlock->superBlock;

    return;
    ERROR_FINAL_BEGIN(0);
    if (FAT != NULL) {
        memory_free(FAT);
    }

    if (buffer != NULL) {
        memory_free(buffer);
    }
    
    if (batchAllocated != NULL) {
        memory_free(batchAllocated);
    }
}

void fat32_close(FS* fs) {
    SuperBlock* superBlock = fs->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);

    superBlock_rawSync(superBlock);
    ERROR_GOTO_IF_ERROR(0);

    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Device* superBlockDevice = &superBlockBlockDevice->device;
    Size FATsizeInByte = fat32SuperBlock->FATrange.length * POWER_2(superBlockDevice->granularity);
    memory_memset(fat32SuperBlock->FAT, 0, FATsizeInByte);
    // memory_freeFrames(paging_convertAddressV2P(fat32SuperBlock->FAT));
    memory_free(fat32SuperBlock->FAT);

    void* batchAllocated = fat32SuperBlock; //TODO: Ugly code
    memory_memset(batchAllocated, 0, __FS_FAT32_BATCH_ALLOCATE_SIZE);
    memory_free(batchAllocated);

    return;
    ERROR_FINAL_BEGIN(0);
}

void fat32SuperBlock_registerMetadata(FAT32SuperBlock* superBlock, DirectoryEntry* entry, fsNode* belongTo, Index64 firstCluster, iNodeAttribute* inodeAttribute) {
    FAT32NodeMetadata* metadata = NULL;
    metadata = memory_allocate(sizeof(FAT32NodeMetadata));
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    string_initStructStr(&metadata->name, entry->name);
    ERROR_GOTO_IF_ERROR(0);
    metadata->type = entry->type;
    metadata->node = NULL;
    metadata->belongTo = belongTo;
    memory_memcpy(&metadata->inodeAttribute, inodeAttribute, sizeof(iNodeAttribute));
    metadata->size = entry->size;

    hashTable_insert(&superBlock->metadataTableInodeID, entry->inodeID, &metadata->hashNodeInodeID);
    hashTable_insert(&superBlock->metadataTableFirstCluster, firstCluster, &metadata->hashNodeFirstCluster);
    ERROR_CHECKPOINT({ 
            ERROR_GOTO(0);
        },
        (ERROR_ID_ALREADY_EXIST, {
            ERROR_CLEAR();
            memory_free(metadata);
        })
    );

    return;
    ERROR_FINAL_BEGIN(0);
    if (metadata != NULL) {
        memory_free(metadata);
    }
}

void fat32SuperBlock_unregisterMetadata(FAT32SuperBlock* superBlock, Index64 firstCluster) {
    HashChainNode* deleted = hashTable_delete(&superBlock->metadataTableFirstCluster, firstCluster);
    if (deleted == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32NodeMetadata* metadata = HOST_POINTER(deleted, FAT32NodeMetadata, hashNodeFirstCluster);
    deleted = hashTable_delete(&superBlock->metadataTableInodeID, FAT32_NODE_METADATA_GET_INODE_ID(metadata));
    if (deleted == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    memory_free(metadata);

    return;
    ERROR_FINAL_BEGIN(0);
}

FAT32NodeMetadata* fat32SuperBlock_getMetadataFromInodeID(FAT32SuperBlock* superBlock, ID inodeID) {
    HashChainNode* found = hashTable_find(&superBlock->metadataTableInodeID, inodeID);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    return HOST_POINTER(found, FAT32NodeMetadata, hashNodeInodeID);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

FAT32NodeMetadata* fat32SuperBlock_getMetadataFromFirstCluster(FAT32SuperBlock* superBlock, Index64 firstCluster) {
    HashChainNode* found = hashTable_find(&superBlock->metadataTableFirstCluster, firstCluster);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    return HOST_POINTER(found, FAT32NodeMetadata, hashNodeFirstCluster);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

Index32 fat32SuperBlock_createFirstCluster(FAT32SuperBlock* superBlock) {
    void* clusterBuffer = NULL;
    Index32 firstCluster = fat32_allocateClusterChain(superBlock, 1);   //First cluster of new file/directory must be all 0
    if (firstCluster == INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32BPB* BPB = superBlock->BPB;
    BlockDevice* targetBlockDevice = superBlock->superBlock.blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    Size clusterSize = BPB->sectorPerCluster * POWER_2(targetDevice->granularity);

    clusterBuffer = memory_allocate(clusterSize);
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    memory_memset(clusterBuffer, 0, clusterSize);
    blockDevice_writeBlocks(targetBlockDevice, superBlock->dataBlockRange.begin + firstCluster * BPB->sectorPerCluster, clusterBuffer, BPB->sectorPerCluster);
    ERROR_GOTO_IF_ERROR(0);

    return firstCluster;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX;
}

static fsNode* __fat32_superBlock_getFSnode(SuperBlock* superBlock, ID inodeID) {
    fsNode* node = NULL;

    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32NodeMetadata* metadata = fat32SuperBlock_getMetadataFromInodeID(fat32SuperBlock, inodeID);
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(inodeID == FAT32_NODE_METADATA_GET_INODE_ID(metadata));
    
    if (metadata->node != NULL) {
        fsNode_refer(metadata->node);
        return metadata->node;
    }

    node = fsNode_create(metadata->name.data, metadata->type, metadata->belongTo, inodeID);
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

static iNode* __fat32_superBlock_openInode(SuperBlock* superBlock, ID inodeID) {
    FAT32Inode* fat32Inode = NULL;

    fat32Inode = memory_allocate(sizeof(FAT32Inode));
    if (fat32Inode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32NodeMetadata* metadata = fat32SuperBlock_getMetadataFromInodeID(fat32SuperBlock, inodeID);
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FAT32BPB* BPB               = fat32SuperBlock->BPB;
    Device* superBlockDevice    = &superBlock->blockDevice->device;
    fat32Inode->firstCluster    = FAT32_NODE_METADATA_GET_FIRST_CLUSTER(metadata);
    fat32Inode->isTouched       = metadata->isTouched;

    iNode* inode = &fat32Inode->inode;
    inode->sizeInByte       = metadata->size;
    inode->sizeInBlock      = fat32_getClusterChainLength(fat32SuperBlock, fat32Inode->firstCluster) * BPB->sectorPerCluster;
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    DEBUG_ASSERT_SILENT(inode->sizeInByte <= inode->sizeInBlock * POWER_2(superBlockBlockDevice->device.granularity));
    
    inode->signature        = INODE_SIGNATURE;
    inode->inodeID          = inodeID;
    inode->superBlock       = superBlock;
    inode->operations       = fat32_iNode_getOperations();
    
    refCounter_initStruct(&inode->refCounter);
    hashChainNode_initStruct(&inode->openedNode);

    inode->fsNode = superBlock_getFSnode(superBlock, inodeID);
    if (inode->fsNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    memory_memcpy(&inode->attribute, &metadata->inodeAttribute, sizeof(iNodeAttribute));

    inode->deviceID         = INVALID_ID;
    inode->lock             = SPINLOCK_UNLOCKED;
    
    if (inode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY && !fat32Inode->isTouched) {
        DEBUG_ASSERT_SILENT(inode->sizeInByte == 0);
        inode->sizeInByte   = fat32_iNode_touchDirectory(inode);
        ERROR_GOTO_IF_ERROR(0);
        fat32Inode->isTouched = metadata->isTouched = true;
    }
    
    inode->fsNode->isInodeActive = true;
    
    return inode;
    ERROR_FINAL_BEGIN(0);
    if (fat32Inode != NULL) {
        memory_free(fat32Inode);
    }
    
    return NULL;
}

static iNode* __fat32_superBlock_openRootInode(SuperBlock* superBlock) {
    FAT32Inode* fat32Inode = NULL;
    
    ID inodeID = superBlock_allocateInodeID(superBlock);
    fsNode* rootNode = fsNode_create("", FS_ENTRY_TYPE_DIRECTORY, NULL, inodeID);

    fat32Inode = memory_allocate(sizeof(FAT32Inode));
    if (fat32Inode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    iNode* inode = &fat32Inode->inode;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32BPB* BPB = fat32SuperBlock->BPB;

    fat32Inode->firstCluster    = BPB->rootDirectoryClusterIndex;
    fat32Inode->isTouched       = false;
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Size blockSize = POWER_2(superBlockBlockDevice->device.granularity);
    inode->sizeInBlock          = fat32_getClusterChainLength(fat32SuperBlock, BPB->rootDirectoryClusterIndex) * BPB->sectorPerCluster;

    inode->signature            = INODE_SIGNATURE;
    inode->inodeID              = inodeID;
    inode->superBlock           = superBlock;
    inode->operations           = fat32_iNode_getOperations();
    
    refCounter_initStruct(&inode->refCounter);
    hashChainNode_initStruct(&inode->openedNode);
    inode->fsNode = rootNode;
    fsNode_refer(inode->fsNode);
    
    inode->attribute = (iNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };
    
    inode->deviceID             = INVALID_ID;
    inode->lock                 = SPINLOCK_UNLOCKED;
    refCounter_refer(&inode->refCounter);
    
    rootNode->isInodeActive = true; //TODO: Ugly code
    
    inode->sizeInByte           = fat32_iNode_touchDirectory(inode);
    ERROR_GOTO_IF_ERROR(0);

    DEBUG_ASSERT_SILENT(inode->sizeInByte <= inode->sizeInBlock * blockSize);

    return inode;
    ERROR_FINAL_BEGIN(0);
    if (fat32Inode != NULL) {
        memory_free(fat32Inode);
    }

    return NULL;
}

static void __fat32_superBlock_closeInode(SuperBlock* superBlock, iNode* inode) {
    if (!refCounter_derefer(&inode->refCounter)) {
        return;
    }
    
    FAT32Inode* fat32Inode = HOST_POINTER(inode, FAT32Inode, inode);
    fsNode_release(inode->fsNode);

    memory_free(fat32Inode);
}

static void __fat32_superBlock_sync(SuperBlock* superBlock) {
    Index32* FATcopy = NULL;

    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    
    FAT32BPB* BPB = fat32SuperBlock->BPB;
    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    Device* superBlockDevice = &superBlockBlockDevice->device;

    RangeN* fatRange = &fat32SuperBlock->FATrange;
    Size FATsizeInByte = fatRange->length * POWER_2(superBlockDevice->granularity);
    FATcopy = memory_allocate(FATsizeInByte);
    if (FATcopy == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memcpy(FATcopy, fat32SuperBlock->FAT, FATsizeInByte);

    for (int i = fat32SuperBlock->firstFreeCluster, next; i != FAT32_CLSUTER_END_OF_CHAIN; i = next) {
        next = PTR_TO_VALUE(32, FATcopy + i);
        PTR_TO_VALUE(32, FATcopy + i) = 0;
    }

    for (int i = 0; i < fatRange->n; ++i) { //TODO: FAT is only saved here, maybe we can make it save at anytime
        blockDevice_writeBlocks(superBlockBlockDevice, fatRange->begin + i * fatRange->length, FATcopy, fatRange->length);
        ERROR_GOTO_IF_ERROR(0);
    }

    memory_free(FATcopy);
    FATcopy = NULL;

    blockDevice_flush(superBlockBlockDevice);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
    if (FATcopy != NULL) {
        memory_free(FATcopy);
    }
    return;
}

static fsEntry* __fat32_superBlock_openFSentry(SuperBlock* superBlock, iNode* inode, FCNTLopenFlags flags) {
    fsEntry* ret = superBlock_genericOpenFSentry(superBlock, inode, flags);
    ERROR_GOTO_IF_ERROR(0);

    ret->operations = &_fat32_fsEntryOperations;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}