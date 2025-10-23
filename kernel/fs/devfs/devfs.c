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
    Index64 mappingIndex = devfscore_allocateMappingIndex();

    FSnodeAttribute attribute;
    fsnodeAttribute_initDefault(&attribute);

    devfsDirectoryEntry_initStruct(&devfscore->rootDirEntry, "", FS_ENTRY_TYPE_DIRECTORY, mappingIndex, (Object)NULL, &attribute);
    devfscore_setStorageMapping(mappingIndex, &devfscore->rootDirEntry);

    FScoreInitArgs args = {
        .blockDevice        = blockDevice,
        .operations         = &__devfs_fscoreOperations,
        .rootFSnodePointsTo = mappingIndex
        // .openedVnodeBucket  = __DEVFS_FSCORE_HASH_BUCKET,
        // .openedVnodeChains  = openedVnodeChains
    };

    fscore_initStruct(fscore, &args);

    fs->fscore = fscore;
    fs->name = __devfs_name;
    fs->type = FS_TYPE_DEVFS;

    vNode* rootVnode = fscore_getVnode(fscore, fscore->rootFSnode, false);
    for (MajorDeviceID major = device_iterateMajor(DEVICE_INVALID_ID); major != DEVICE_INVALID_ID; major = device_iterateMajor(major)) {    //TODO: What if device joins after boot?
        for (Device* device = device_iterateMinor(major, DEVICE_INVALID_ID); device != NULL; device = device_iterateMinor(major, DEVICE_MINOR_FROM_ID(device->id))) {
            DirectoryEntry newEntry = (DirectoryEntry) {
                .name = device->name,
                .type = FS_ENTRY_TYPE_DEVICE,
                .mode = 0,  //TODO: mode not used yet
                .vnodeID = DIRECTORY_ENTRY_VNDOE_ID_ANY,
                .size = DIRECTORY_ENTRY_SIZE_ANY,
                .pointsTo = device->id
            };
            vNode_addDirectoryEntry(rootVnode, &newEntry, &attribute);
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

Index64 devfscore_allocateMappingIndex() {
    Index64 ret = _devfs_storageMappingFirstFree;
    if (ret == INVALID_INDEX64) {
        ret = _devfs_storageMapping.size;
    } else {
        _devfs_storageMappingFirstFree = vector_get(&_devfs_storageMapping, _devfs_storageMappingFirstFree);
    }

    return ret;
}

void devfscore_releaseMappingIndex(Index64 mappingIndex) {
    vector_set(&_devfs_storageMapping, mappingIndex, _devfs_storageMappingFirstFree);
    _devfs_storageMappingFirstFree = mappingIndex;
}

DevfsDirectoryEntry* devfscore_getStorageMapping(Index64 mappingIndex) {
    return (DevfsDirectoryEntry*)vector_get(&_devfs_storageMapping, mappingIndex);
}

void devfscore_setStorageMapping(Index64 mappingIndex, DevfsDirectoryEntry* mapTo) {
    DEBUG_ASSERT_SILENT(mapTo->mappingIndex == mappingIndex);
    vector_set(&_devfs_storageMapping, mappingIndex, (Object)mapTo);
}

static vNode* __devfs_fscore_openVnode(FScore* fscore, fsNode* node) {
    DevfsVnode* devfsVnode = NULL;

    devfsVnode = mm_allocate(sizeof(DevfsVnode));
    if (devfsVnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Devfscore* devfscore = HOST_POINTER(fscore, Devfscore, fscore);
    DirectoryEntry* nodeEntry = &node->entry;

    vNode* vnode = &devfsVnode->vnode;

    DevfsDirectoryEntry* dirEntry = devfscore_getStorageMapping(nodeEntry->pointsTo);
    DEBUG_ASSERT_SILENT(dirEntry->mappingIndex == (Index64)nodeEntry->pointsTo && dirEntry->size == nodeEntry->size);

    vnode->signature            = VNODE_SIGNATURE;
    vnode->vnodeID              = node->entry.vnodeID;
    vnode->size                 = nodeEntry->size;
    
    fsEntryType type = nodeEntry->type;
    if (type == FS_ENTRY_TYPE_FILE || type == FS_ENTRY_TYPE_DIRECTORY) {
        vnode->tokenSpaceSize   = ALIGN_UP(dirEntry->size, PAGE_SIZE);
        vnode->deviceID         = INVALID_ID;
        devfsVnode->data        = (void*)dirEntry->pointsTo;
    } else {
        vnode->tokenSpaceSize   = INFINITE;
        vnode->deviceID         = (ID)dirEntry->pointsTo;
        devfsVnode->data        = NULL;
    }
    DEBUG_ASSERT_SILENT(vnode->size <= vnode->tokenSpaceSize);

    vnode->fscore           = fscore;
    vnode->operations       = devfs_vNode_getOperations();

    REF_COUNTER_INIT(vnode->refCounter, 0);
    
    vnode->fsNode           = node;
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