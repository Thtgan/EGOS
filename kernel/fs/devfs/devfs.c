#include<fs/devfs/devfs.h>

#include<devices/block/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/superblock.h>
#include<fs/devfs/blockChain.h>
#include<fs/devfs/inode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<multitask/locks/spinlock.h>
#include<structs/hashTable.h>
#include<structs/refCounter.h>
#include<error.h>

static iNode* __devfs_superBlock_openInode(SuperBlock* superBlock, ID inodeID);

static iNode* __devfs_superBlock_openRootInode(SuperBlock* superBlock);

static void __devfs_superBlock_closeInode(SuperBlock* superBlock, iNode* inode);

static void __devfs_superBlock_sync(SuperBlock* superBlock);

static fsEntry* __devfs_superBlock_openFSentry(SuperBlock* superBlock, iNode* inode, FCNTLopenFlags flags);

static ConstCstring __devfs_name = "DEVFS";
static SuperBlockOperations __devfs_superBlockOperations = {
    .openInode      = __devfs_superBlock_openInode,
    .openRootInode  = __devfs_superBlock_openRootInode,
    .closeInode     = __devfs_superBlock_closeInode,
    .sync           = __devfs_superBlock_sync,
    .openFSentry    = __devfs_superBlock_openFSentry,
    .closeFSentry   = superBlock_genericCloseFSentry,
    .mount          = superBlock_genericMount,
    .unmount        = superBlock_genericUnmount
};

static fsEntryOperations _devfs_fsEntryOperations = {
    .seek           = fsEntry_genericSeek,
    .read           = fsEntry_genericRead,
    .write          = fsEntry_genericWrite
};

//TODO: Capsule these data
static bool _devfs_opened = false;

void devfs_init() {
}

bool devfs_checkType(BlockDevice* blockDevice) {
    return true;  //TODO: Not a good logic
}

#define __DEVFS_SUPERBLOCK_HASH_BUCKET  31
#define __DEVFS_BATCH_ALLOCATE_SIZE     BATCH_ALLOCATE_SIZE((DevfsSuperBlock, 1), (SinglyLinkedList, __DEVFS_SUPERBLOCK_HASH_BUCKET))

void devfs_open(FS* fs, BlockDevice* blockDevice) {
    void* batchAllocated = NULL;
    Device* device = &blockDevice->device;
    if (_devfs_opened || device->capacity != DEVFS_BLOCK_CHAIN_CAPACITY) { //TODO: Make it flexible
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    batchAllocated = memory_allocate(__DEVFS_BATCH_ALLOCATE_SIZE);
    if (batchAllocated == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (DevfsSuperBlock, devfsSuperBlock, 1),
        (SinglyLinkedList, openedInodeChains, __DEVFS_SUPERBLOCK_HASH_BUCKET)
    );

    SuperBlock* superBlock = &devfsSuperBlock->superBlock;
    DevfsBlockChains* blockChains = &devfsSuperBlock->blockChains;
    devfs_blockChain_initStruct(blockChains);

    hashTable_initStruct(&devfsSuperBlock->metadataTable, DEVFS_SUPERBLOCK_INODE_TABLE_CHAIN_NUM, devfsSuperBlock->metadataTableChains, hashTable_defaultHashFunc);

    SuperBlockInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__devfs_superBlockOperations,
        .openedInodeBucket  = __DEVFS_SUPERBLOCK_HASH_BUCKET,
        .openedInodeChains  = openedInodeChains
    };

    superBlock_initStruct(superBlock, &args);

    fs->superBlock = superBlock;
    fs->name = __devfs_name;
    fs->type = FS_TYPE_DEVFS;

    _devfs_opened = true;
    
    return;
    ERROR_FINAL_BEGIN(0);
    if (batchAllocated != NULL) {
        memory_free(batchAllocated);
    }
}

void devfs_close(FS* fs) {
    _devfs_opened = false;
    memory_free(fs->superBlock);
}

void devfsSuperBlock_registerMetadata(DevfsSuperBlock* superBlock, ID inodeID, fsNode* node, Size sizeInByte, Object pointsTo) {
    DevfsNodeMetadata* metadata = NULL;
    metadata = memory_allocate(sizeof(DevfsNodeMetadata));
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    metadata->node = node;
    metadata->sizeInByte = sizeInByte ;
    metadata->pointsTo = pointsTo;

    fsNode_refer(node);

    hashTable_insert(&superBlock->metadataTable, inodeID, &metadata->hashNode);
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

void devfsSuperBlock_unregisterMetadata(DevfsSuperBlock* superBlock, ID inodeID) {
    HashChainNode* deleted = hashTable_delete(&superBlock->metadataTable, inodeID);
    if (deleted == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    DevfsNodeMetadata* node = HOST_POINTER(deleted, DevfsNodeMetadata, hashNode);

    fsNode_release(node->node);
    memory_free(node);

    return;
    ERROR_FINAL_BEGIN(0);
}

DevfsNodeMetadata* devfsSuperBlock_getMetadata(DevfsSuperBlock* superBlock, ID inodeID) {
    HashChainNode* found = hashTable_find(&superBlock->metadataTable, inodeID);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    return HOST_POINTER(found, DevfsNodeMetadata, hashNode);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static iNode* __devfs_superBlock_openInode(SuperBlock* superBlock, ID inodeID) {
    DevfsInode* devfsInode = NULL;

    devfsInode = memory_allocate(sizeof(DevfsInode));
    if (devfsInode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(superBlock, DevfsSuperBlock, superBlock);
    HashChainNode* found = hashTable_find(&devfsSuperBlock->metadataTable, inodeID);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    iNode* inode = &devfsInode->inode;
    DevfsNodeMetadata* metadata = HOST_POINTER(found, DevfsNodeMetadata, hashNode);
    if (metadata->node->type == FS_ENTRY_TYPE_DEVICE) {
        inode->sizeInByte       = INFINITE;
        inode->sizeInBlock      = INFINITE;
        inode->deviceID         = (ID)metadata->pointsTo;
    } else {
        DevfsBlockChains* chains = &devfsSuperBlock->blockChains;
        devfsInode->firstBlock  = (Index8)metadata->pointsTo;
        inode->sizeInByte       = metadata->sizeInByte;
        Uint64 sizeInBlock      = devfs_blockChain_getChainLength(chains, devfsInode->firstBlock);
        inode->sizeInBlock      = sizeInBlock;

        BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
        DEBUG_ASSERT_SILENT(inode->sizeInByte <= inode->sizeInBlock * POWER_2(superBlockBlockDevice->device.granularity));
    }

    inode->signature        = INODE_SIGNATURE;
    inode->inodeID          = inodeID;
    inode->superBlock       = superBlock;
    inode->operations       = devfs_iNode_getOperations();

    refCounter_initStruct(&inode->refCounter);
    hashChainNode_initStruct(&inode->openedNode);

    inode->attribute = (iNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };

    inode->fsNode           = metadata->node;
    fsNode_refer(inode->fsNode);
    inode->lock             = SPINLOCK_UNLOCKED;

    return inode;
    ERROR_FINAL_BEGIN(0);
    if (devfsInode != NULL) {
        memory_free(devfsInode);
    }

    return NULL;
}

static iNode* __devfs_superBlock_openRootInode(SuperBlock* superBlock) {
    DevfsInode* devfsInode = NULL;
    
    fsNode* rootNode = fsNode_create("", FS_ENTRY_TYPE_DIRECTORY, NULL);
    ID inodeID = fsNode_getInodeID(rootNode, superBlock);

    devfsInode = memory_allocate(sizeof(DevfsInode));
    if (devfsInode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    iNode* inode = &devfsInode->inode;

    devfsInode->firstBlock  = INVALID_INDEX;
    inode->sizeInByte       = 0;
    inode->sizeInBlock      = 0;

    BlockDevice* superBlockBlockDevice = superBlock->blockDevice;
    DEBUG_ASSERT_SILENT(inode->sizeInByte <= inode->sizeInBlock * POWER_2(superBlockBlockDevice->device.granularity));

    inode->signature        = INODE_SIGNATURE;
    inode->inodeID          = inodeID;
    inode->superBlock       = superBlock;
    inode->operations       = devfs_iNode_getOperations();

    refCounter_initStruct(&inode->refCounter);
    hashChainNode_initStruct(&inode->openedNode);

    inode->attribute = (iNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };

    inode->fsNode = rootNode;
    fsNode_refer(inode->fsNode);
    inode->lock             = SPINLOCK_UNLOCKED;
    refCounter_refer(&inode->refCounter);

    rootNode->isInodeActive = true; //TODO: Ugly code

    iNodeAttribute attribute = (iNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };

    for (MajorDeviceID major = device_iterateMajor(DEVICE_INVALID_ID); major != DEVICE_INVALID_ID; major = device_iterateMajor(major)) {    //TODO: What if device joins after boot?
        for (Device* device = device_iterateMinor(major, DEVICE_INVALID_ID); device != NULL; device = device_iterateMinor(major, DEVICE_MINOR_FROM_ID(device->id))) {
            iNode_rawAddDirectoryEntry(inode, device->name, FS_ENTRY_TYPE_DEVICE, &attribute, device->id);
            ERROR_GOTO_IF_ERROR(0);
        }
    }

    return inode;
    ERROR_FINAL_BEGIN(0);
    if (devfsInode != NULL) {
        memory_free(devfsInode);
    }

    return NULL;
}

static void __devfs_superBlock_closeInode(SuperBlock* superBlock, iNode* inode) {
    DevfsInode* devfsInode = HOST_POINTER(inode, DevfsInode, inode);
    fsNode_release(inode->fsNode);

    memory_free(devfsInode);
}

static void __devfs_superBlock_sync(SuperBlock* superBlock) {

}

static fsEntry* __devfs_superBlock_openFSentry(SuperBlock* superBlock, iNode* inode, FCNTLopenFlags flags) {
    fsEntry* ret = superBlock_genericOpenFSentry(superBlock, inode, flags);
    ERROR_GOTO_IF_ERROR(0);

    ret->operations = &_devfs_fsEntryOperations;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}