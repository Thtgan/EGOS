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
#include<structs/vector.h>
#include<error.h>

static vNode* __devfs_fscore_openVnode(FScore* fscore, fsNode* node);

static void __devfs_fscore_closeVnode(FScore* fscore, vNode* vnode);

static void __devfs_fscore_sync(FScore* fscore);

static fsEntry* __devfs_fscore_openFSentry(FScore* fscore, vNode* vnode, FCNTLopenFlags flags);

static ConstCstring __devfs_name = "DEVFS";
static FScoreOperations __devfs_fscoreOperations = {
    .openVnode      = __devfs_fscore_openVnode,
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

static Vector _devfs_storageMapping;
static Index64 _devfs_storageMappingFirstFree;

//TODO: Capsule these data
static bool _devfs_opened = false;

void devfs_init() {
}

bool devfs_checkType(BlockDevice* blockDevice) {
    return blockDevice == NULL; //Only devfs accepts NULL device (probably)
}

// #define __DEVFS_FSCORE_HASH_BUCKET  31
// #define __DEVFS_BATCH_ALLOCATE_SIZE     BATCH_ALLOCATE_SIZE((Devfscore, 1), (SinglyLinkedList, __DEVFS_FSCORE_HASH_BUCKET))
#define __DEVFS_BATCH_ALLOCATE_SIZE     BATCH_ALLOCATE_SIZE((Devfscore, 1))

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
        // (Devfscore, devfscore, 1),
        // (SinglyLinkedList, openedVnodeChains, __DEVFS_FSCORE_HASH_BUCKET)
        (Devfscore, devfscore, 1)
    );

    FScore* fscore = &devfscore->fscore;

    vector_initStructN(&_devfs_storageMapping, 32);
    for (int i = 1; i < 32; ++i) {
        vector_push(&_devfs_storageMapping, (Object)i);
    }
    vector_push(&_devfs_storageMapping, (Object)INVALID_INDEX64);
    _devfs_storageMappingFirstFree = 0;

    hashTable_initStruct(&devfscore->metadataTable, DEVFS_FSCORE_VNODE_TABLE_CHAIN_NUM, devfscore->metadataTableChains, hashTable_defaultHashFunc);

    Index64 mappingIndex = devfscore_registerMetadata(devfscore, fscore->rootFSnode, 0, (Object)NULL);

    FScoreInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__devfs_fscoreOperations,
        .rootFSnodePosition = mappingIndex
        // .openedVnodeBucket  = __DEVFS_FSCORE_HASH_BUCKET,
        // .openedVnodeChains  = openedVnodeChains
    };

    fscore_initStruct(fscore, &args);

    fs->fscore = fscore;
    fs->name = __devfs_name;
    fs->type = FS_TYPE_DEVFS;

    vNode* rootVnode = fscore_getVnode(fscore, fscore->rootFSnode, false);
    vNodeAttribute attribute = (vNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };

    for (MajorDeviceID major = device_iterateMajor(DEVICE_INVALID_ID); major != DEVICE_INVALID_ID; major = device_iterateMajor(major)) {    //TODO: What if device joins after boot?
        for (Device* device = device_iterateMinor(major, DEVICE_INVALID_ID); device != NULL; device = device_iterateMinor(major, DEVICE_MINOR_FROM_ID(device->id))) {
            vNode_addDirectoryEntry(rootVnode, device->name, FS_ENTRY_TYPE_DEVICE, &attribute, device->id);
            ERROR_GOTO_IF_ERROR(0);
        }
    }
    fscore_releaseVnode(rootVnode);

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

Index64 devfscore_registerMetadata(Devfscore* fscore, fsNode* node, Size sizeInByte, Object pointsTo) {
    DevfsNodeMetadata* metadata = NULL;
    metadata = mm_allocate(sizeof(DevfsNodeMetadata));
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    metadata->node = node;
    metadata->sizeInByte = sizeInByte;

    Index64 ret = _devfs_storageMappingFirstFree;
    if (ret == INVALID_INDEX64) {
        ret = _devfs_storageMapping.size;
        vector_push(&_devfs_storageMapping, (Object)pointsTo);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        _devfs_storageMappingFirstFree = vector_get(&_devfs_storageMapping, _devfs_storageMappingFirstFree);
        vector_set(&_devfs_storageMapping, ret, (Object)pointsTo);
    }

    hashTable_insert(&fscore->metadataTable, (Object)ret, &metadata->hashNode);
    ERROR_CHECKPOINT({ 
            ERROR_GOTO(0);
        },
        (ERROR_ID_ALREADY_EXIST, {
            ERROR_CLEAR();
            mm_free(metadata);
        })
    );

    return ret;
    ERROR_FINAL_BEGIN(0);
    if (metadata != NULL) {
        mm_free(metadata);
    }

    return INVALID_INDEX64;
}

void devfscore_unregisterMetadata(Devfscore* fscore, Index64 mappingIndex) {
    HashChainNode* deleted = hashTable_delete(&fscore->metadataTable, mappingIndex);
    if (deleted == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    DevfsNodeMetadata* node = HOST_POINTER(deleted, DevfsNodeMetadata, hashNode);

    mm_free(node);

    vector_set(&_devfs_storageMapping, mappingIndex, _devfs_storageMappingFirstFree);
    _devfs_storageMappingFirstFree = mappingIndex;

    return;
    ERROR_FINAL_BEGIN(0);
}

DevfsNodeMetadata* devfscore_getMetadata(Devfscore* fscore, Index64 mappingIndex) {
    HashChainNode* found = hashTable_find(&fscore->metadataTable, mappingIndex);
    if (found == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    return HOST_POINTER(found, DevfsNodeMetadata, hashNode);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

Object devfscore_getStorageMapping(Index64 mappingIndex) {
    return vector_get(&_devfs_storageMapping, mappingIndex);
}

void devfscore_setStorageMapping(Index64 mappingIndex, Object mapTo) {
    vector_set(&_devfs_storageMapping, mappingIndex, mapTo);
}

static vNode* __devfs_fscore_openVnode(FScore* fscore, fsNode* node) {
    DevfsVnode* devfsVnode = NULL;

    devfsVnode = mm_allocate(sizeof(DevfsVnode));
    if (devfsVnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Devfscore* devfscore = HOST_POINTER(fscore, Devfscore, fscore);

    vNode* vnode = &devfsVnode->vnode;
    DevfsNodeMetadata* metadata = devfscore_getMetadata(devfscore, (ID)node->physicalPosition);
    ERROR_GOTO_IF_ERROR(0);

    vnode->signature        = VNODE_SIGNATURE;
    vnode->vnodeID          = (ID)vnode;    //TODO: Ugly code
    fsEntryType type = node->type;
    if (type == FS_ENTRY_TYPE_FILE || type == FS_ENTRY_TYPE_DIRECTORY) {
        vnode->sizeInByte   = metadata->sizeInByte;
        vnode->sizeInBlock  = 0;
        vnode->deviceID     = INVALID_ID;
        devfsVnode->data    = (void*)devfscore_getStorageMapping(node->physicalPosition);
    } else {
        vnode->sizeInByte   = INFINITE;
        vnode->sizeInBlock  = INFINITE;
        vnode->deviceID     = (ID)devfscore_getStorageMapping(node->physicalPosition);
        devfsVnode->data    = NULL;
    }

    vnode->fscore           = fscore;
    vnode->operations       = devfs_vNode_getOperations();

    REF_COUNTER_INIT(vnode->refCounter, 0);
    
    vnode->fsNode           = node;
    vnode->attribute        = (vNodeAttribute) {   //TODO: Ugly code
        .uid = 0,
        .gid = 0,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };
    vnode->lock             = SPINLOCK_UNLOCKED;

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