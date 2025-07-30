#include<fs/devfs/devfs.h>

#include<devices/blockDevice.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/fscore.h>
#include<fs/devfs/vnode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/mm.h>
#include<multitask/locks/spinlock.h>
#include<structs/hashTable.h>
#include<structs/refCounter.h>
#include<error.h>

static fsNode* __devfs_fscore_getFSnode(FScore* fscore, ID vnodeID);

static vNode* __devfs_fscore_openVnode(FScore* fscore, ID vnodeID);

static vNode* __devfs_fscore_openRootVnode(FScore* fscore);

static void __devfs_fscore_closeVnode(FScore* fscore, vNode* vnode);

static void __devfs_fscore_sync(FScore* fscore);

static fsEntry* __devfs_fscore_openFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags);

static ConstCstring __devfs_name = "DEVFS";
static FScoreOperations __devfs_fscoreOperations = {
    .getFSnode      = __devfs_fscore_getFSnode,
    .openVnode      = __devfs_fscore_openVnode,
    .openRootVnode  = __devfs_fscore_openRootVnode,
    .closeVnode     = __devfs_fscore_closeVnode,
    .sync           = __devfs_fscore_sync,
    .openFSentry    = __devfs_fscore_openFSentry,
    .closeFSentry   = fscore_genericCloseFSentry,
    .mount          = fscore_genericMount,
    .unmount        = fscore_genericUnmount
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

#define __DEVFS_FSCORE_HASH_BUCKET  31
#define __DEVFS_BATCH_ALLOCATE_SIZE     BATCH_ALLOCATE_SIZE((Devfscore, 1), (SinglyLinkedList, __DEVFS_FSCORE_HASH_BUCKET))

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
        (Devfscore, devfscore, 1),
        (SinglyLinkedList, openedVnodeChains, __DEVFS_FSCORE_HASH_BUCKET)
    );

    FScore* fscore = &devfscore->fscore;
    hashTable_initStruct(&devfscore->metadataTable, DEVFS_FSCORE_VNODE_TABLE_CHAIN_NUM, devfscore->metadataTableChains, hashTable_defaultHashFunc);

    FScoreInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__devfs_fscoreOperations,
        .openedVnodeBucket  = __DEVFS_FSCORE_HASH_BUCKET,
        .openedVnodeChains  = openedVnodeChains
    };

    fscore_initStruct(fscore, &args);

    fs->fscore = fscore;
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
    mm_free(fs->fscore);
}

void devfscore_registerMetadata(Devfscore* fscore, ID vnodeID, fsNode* node, Size sizeInByte, Object pointsTo) {
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

    hashTable_insert(&fscore->metadataTable, vnodeID, &metadata->hashNode);
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

void devfscore_unregisterMetadata(Devfscore* fscore, ID vnodeID) {
    HashChainNode* deleted = hashTable_delete(&fscore->metadataTable, vnodeID);
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

DevfsNodeMetadata* devfscore_getMetadata(Devfscore* fscore, ID vnodeID) {
    HashChainNode* found = hashTable_find(&fscore->metadataTable, vnodeID);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    return HOST_POINTER(found, DevfsNodeMetadata, hashNode);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static fsNode* __devfs_fscore_getFSnode(FScore* fscore, ID vnodeID) {
    Devfscore* devfscore = HOST_POINTER(fscore, Devfscore, fscore);
    HashChainNode* found = hashTable_find(&devfscore->metadataTable, vnodeID);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    DevfsNodeMetadata* metadata = HOST_POINTER(found, DevfsNodeMetadata, hashNode);
    DEBUG_ASSERT_SILENT(metadata->node != NULL);
    
    return metadata->node;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static vNode* __devfs_fscore_openVnode(FScore* fscore, ID vnodeID) {
    DevfsVnode* devfsVnode = NULL;

    devfsVnode = mm_allocate(sizeof(DevfsVnode));
    if (devfsVnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Devfscore* devfscore = HOST_POINTER(fscore, Devfscore, fscore);
    HashChainNode* found = hashTable_find(&devfscore->metadataTable, vnodeID);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    vNode* vnode = &devfsVnode->vnode;
    DevfsNodeMetadata* metadata = HOST_POINTER(found, DevfsNodeMetadata, hashNode);
    DEBUG_ASSERT_SILENT(metadata->node != NULL);

    vnode->signature        = VNODE_SIGNATURE;
    vnode->vnodeID          = vnodeID;
    fsEntryType type = metadata->node->type;
    if (type == FS_ENTRY_TYPE_FILE || type == FS_ENTRY_TYPE_DIRECTORY) {
        vnode->sizeInByte   = metadata->sizeInByte;
        vnode->sizeInBlock  = 0;
        vnode->deviceID     = INVALID_ID;
    } else {
        vnode->sizeInByte   = INFINITE;
        vnode->sizeInBlock  = INFINITE;
        vnode->deviceID     = (ID)metadata->pointsTo;
    }

    vnode->fscore       = fscore;
    vnode->operations       = devfs_vNode_getOperations();

    REF_COUNTER_INIT(vnode->refCounter, 0);
    
    vnode->fsNode           = metadata->node;
    vnode->attribute        = (vNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };
    vnode->lock             = SPINLOCK_UNLOCKED;
    
    devfsVnode->data = NULL;

    return vnode;
    ERROR_FINAL_BEGIN(0);
    if (devfsVnode != NULL) {
        mm_free(devfsVnode);
    }

    ERROR_CHECKPOINT();

    return NULL;
}

static vNode* __devfs_fscore_openRootVnode(FScore* fscore) {
    DevfsVnode* devfsVnode = NULL;
    
    ID vnodeID = fscore_allocateVnodeID(fscore);
    fsNode* rootNode = fsNode_create("", FS_ENTRY_TYPE_DIRECTORY, NULL, vnodeID);

    devfsVnode = mm_allocate(sizeof(DevfsVnode));
    if (devfsVnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    vNode* vnode = &devfsVnode->vnode;
    vnode->signature        = VNODE_SIGNATURE;
    vnode->vnodeID          = vnodeID;
    vnode->sizeInByte       = 0;
    vnode->sizeInBlock      = 0;
    vnode->fscore           = fscore;
    vnode->operations       = devfs_vNode_getOperations();

    REF_COUNTER_INIT(vnode->refCounter, 0);
    
    vnode->fsNode           = rootNode;
    vnode->attribute        = (vNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };
    vnode->deviceID         = INVALID_ID;
    vnode->lock             = SPINLOCK_UNLOCKED;
    
    rootNode->isVnodeActive = true; //TODO: Ugly code
    
    vNodeAttribute attribute = (vNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };
    
    devfsVnode->data = NULL;
    
    REF_COUNTER_REFER(vnode->refCounter);
    fsNode_refer(vnode->fsNode);

    for (MajorDeviceID major = device_iterateMajor(DEVICE_INVALID_ID); major != DEVICE_INVALID_ID; major = device_iterateMajor(major)) {    //TODO: What if device joins after boot?
        for (Device* device = device_iterateMinor(major, DEVICE_INVALID_ID); device != NULL; device = device_iterateMinor(major, DEVICE_MINOR_FROM_ID(device->id))) {
            vNode_rawAddDirectoryEntry(vnode, device->name, FS_ENTRY_TYPE_DEVICE, &attribute, device->id);
            ERROR_GOTO_IF_ERROR(0);
        }
    }

    return vnode;
    ERROR_FINAL_BEGIN(0);
    if (devfsVnode != NULL) {
        mm_free(devfsVnode);
    }

    return NULL;
}

static void __devfs_fscore_closeVnode(FScore* fscore, vNode* vnode) {
    if (REF_COUNTER_DEREFER(vnode->refCounter) != 0) {
        return;
    }

    fsNode_release(vnode->fsNode);
    vnode->fsNode->isVnodeActive = false;
    DevfsVnode* devfsVnode = HOST_POINTER(vnode, DevfsVnode, vnode);
    mm_free(devfsVnode);
}

static void __devfs_fscore_sync(FScore* fscore) {

}

static fsEntry* __devfs_fscore_openFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags) {
    fsEntry* ret = fscore_genericOpenFSentry(fscore, vnode, flags);
    ERROR_GOTO_IF_ERROR(0);

    ret->operations = &_devfs_fsEntryOperations;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}