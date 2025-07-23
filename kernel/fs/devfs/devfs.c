#include<fs/devfs/devfs.h>

#include<devices/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/superblock.h>
#include<fs/devfs/inode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/mm.h>
#include<multitask/locks/spinlock.h>
#include<structs/hashTable.h>
#include<structs/refCounter.h>
#include<error.h>

static fsNode* __devfs_superBlock_getFSnode(SuperBlock* superBlock, ID inodeID);

static iNode* __devfs_superBlock_openInode(SuperBlock* superBlock, ID inodeID);

static iNode* __devfs_superBlock_openRootInode(SuperBlock* superBlock);

static void __devfs_superBlock_closeInode(SuperBlock* superBlock, iNode* inode);

static void __devfs_superBlock_sync(SuperBlock* superBlock);

static fsEntry* __devfs_superBlock_openFSentry(SuperBlock* superBlock, iNode* inode, FCNTLopenFlags flags);

static ConstCstring __devfs_name = "DEVFS";
static SuperBlockOperations __devfs_superBlockOperations = {
    .getFSnode      = __devfs_superBlock_getFSnode,
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
    return blockDevice == NULL; //Only devfs accepts NULL device (probably)
}

#define __DEVFS_SUPERBLOCK_HASH_BUCKET  31
#define __DEVFS_BATCH_ALLOCATE_SIZE     BATCH_ALLOCATE_SIZE((DevfsSuperBlock, 1), (SinglyLinkedList, __DEVFS_SUPERBLOCK_HASH_BUCKET))

void devfs_open(FS* fs, BlockDevice* blockDevice) {
    DEBUG_ASSERT_SILENT(blockDevice == NULL);
    
    void* batchAllocated = NULL;
    if (_devfs_opened) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    batchAllocated = mm_allocate(__DEVFS_BATCH_ALLOCATE_SIZE);
    if (batchAllocated == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    BATCH_ALLOCATE_DEFINE_PTRS(batchAllocated, 
        (DevfsSuperBlock, devfsSuperBlock, 1),
        (SinglyLinkedList, openedInodeChains, __DEVFS_SUPERBLOCK_HASH_BUCKET)
    );

    SuperBlock* superBlock = &devfsSuperBlock->superBlock;
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
        mm_free(batchAllocated);
    }
}

void devfs_close(FS* fs) {
    _devfs_opened = false;
    mm_free(fs->superBlock);
}

void devfsSuperBlock_registerMetadata(DevfsSuperBlock* superBlock, ID inodeID, fsNode* node, Size sizeInByte, Object pointsTo) {
    DevfsNodeMetadata* metadata = NULL;
    metadata = mm_allocate(sizeof(DevfsNodeMetadata));
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
            mm_free(metadata);
        })
    );

    return;
    ERROR_FINAL_BEGIN(0);
    if (metadata != NULL) {
        mm_free(metadata);
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
    mm_free(node);

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

static fsNode* __devfs_superBlock_getFSnode(SuperBlock* superBlock, ID inodeID) {
    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(superBlock, DevfsSuperBlock, superBlock);
    HashChainNode* found = hashTable_find(&devfsSuperBlock->metadataTable, inodeID);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    DevfsNodeMetadata* metadata = HOST_POINTER(found, DevfsNodeMetadata, hashNode);
    DEBUG_ASSERT_SILENT(metadata->node != NULL);
    
    return metadata->node;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static iNode* __devfs_superBlock_openInode(SuperBlock* superBlock, ID inodeID) {
    DevfsInode* devfsInode = NULL;

    devfsInode = mm_allocate(sizeof(DevfsInode));
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
    DEBUG_ASSERT_SILENT(metadata->node != NULL);

    inode->signature        = INODE_SIGNATURE;
    inode->inodeID          = inodeID;
    fsEntryType type = metadata->node->type;
    if (type == FS_ENTRY_TYPE_FILE || type == FS_ENTRY_TYPE_DIRECTORY) {
        inode->sizeInByte   = metadata->sizeInByte;
        inode->sizeInBlock  = 0;
        inode->deviceID     = INVALID_ID;
    } else {
        inode->sizeInByte   = INFINITE;
        inode->sizeInBlock  = INFINITE;
        inode->deviceID     = (ID)metadata->pointsTo;
    }

    inode->superBlock       = superBlock;
    inode->operations       = devfs_iNode_getOperations();

    REF_COUNTER_INIT(inode->refCounter, 0);
    
    inode->fsNode           = metadata->node;
    inode->attribute        = (iNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };
    inode->lock             = SPINLOCK_UNLOCKED;
    
    devfsInode->data = NULL;

    return inode;
    ERROR_FINAL_BEGIN(0);
    if (devfsInode != NULL) {
        mm_free(devfsInode);
    }

    ERROR_CHECKPOINT();

    return NULL;
}

static iNode* __devfs_superBlock_openRootInode(SuperBlock* superBlock) {
    DevfsInode* devfsInode = NULL;
    
    ID inodeID = superBlock_allocateInodeID(superBlock);
    fsNode* rootNode = fsNode_create("", FS_ENTRY_TYPE_DIRECTORY, NULL, inodeID);

    devfsInode = mm_allocate(sizeof(DevfsInode));
    if (devfsInode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    iNode* inode = &devfsInode->inode;
    inode->signature        = INODE_SIGNATURE;
    inode->inodeID          = inodeID;
    inode->sizeInByte       = 0;
    inode->sizeInBlock      = 0;
    inode->superBlock       = superBlock;
    inode->operations       = devfs_iNode_getOperations();

    REF_COUNTER_INIT(inode->refCounter, 0);
    
    inode->fsNode           = rootNode;
    inode->attribute        = (iNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };
    inode->deviceID         = INVALID_ID;
    inode->lock             = SPINLOCK_UNLOCKED;
    
    rootNode->isInodeActive = true; //TODO: Ugly code
    
    iNodeAttribute attribute = (iNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };
    
    devfsInode->data = NULL;
    
    REF_COUNTER_REFER(inode->refCounter);
    fsNode_refer(inode->fsNode);

    for (MajorDeviceID major = device_iterateMajor(DEVICE_INVALID_ID); major != DEVICE_INVALID_ID; major = device_iterateMajor(major)) {    //TODO: What if device joins after boot?
        for (Device* device = device_iterateMinor(major, DEVICE_INVALID_ID); device != NULL; device = device_iterateMinor(major, DEVICE_MINOR_FROM_ID(device->id))) {
            iNode_rawAddDirectoryEntry(inode, device->name, FS_ENTRY_TYPE_DEVICE, &attribute, device->id);
            ERROR_GOTO_IF_ERROR(0);
        }
    }

    return inode;
    ERROR_FINAL_BEGIN(0);
    if (devfsInode != NULL) {
        mm_free(devfsInode);
    }

    return NULL;
}

static void __devfs_superBlock_closeInode(SuperBlock* superBlock, iNode* inode) {
    if (REF_COUNTER_DEREFER(inode->refCounter) != 0) {
        return;
    }

    fsNode_release(inode->fsNode);
    inode->fsNode->isInodeActive = false;
    DevfsInode* devfsInode = HOST_POINTER(inode, DevfsInode, inode);
    mm_free(devfsInode);
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