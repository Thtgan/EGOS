#include<fs/fs.h>

#include<devices/blockDevice.h>
#include<devices/memoryBlockDevice.h>
#include<fs/devfs/devfs.h>
#include<fs/ext2/ext2.h>
#include<fs/fat32/fat32.h>
#include<fs/fsEntry.h>
#include<fs/fsIdentifier.h>
#include<fs/path.h>
#include<kit/util.h>
#include<memory/paging.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<structs/hashTable.h>
#include<cstring.h>
#include<error.h>

FS* fs_rootFS = NULL, * fs_devFS = NULL, * fs_ext2;

typedef struct {
    void  (*init)();
    bool  (*checkType)(BlockDevice* device);
    void  (*open)(FS* fs, BlockDevice* device);
    void  (*close)(FS* fs);
} __FileSystemSupport;

static __FileSystemSupport _supports[FS_TYPE_NUM] = {
    [FS_TYPE_FAT32] = {
        .init       = fat32_init,
        .checkType  = fat32_checkType,
        .open       = fat32_open,
        .close      = fat32_close
    },
    [FS_TYPE_EXT2] = {
        .init       = ext2_init,
        .checkType  = ext2_checkType,
        .open       = ext2_open,
        .close      = ext2_close
    },
    [FS_TYPE_DEVFS] = {
        .init       = devfs_init,
        .checkType  = devfs_checkType,
        .open       = devfs_open,
        .close      = devfs_close
    }
};

void fs_init() {
    if (blockDevice_bootFromDevice == NULL) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    FStype type = fs_checkType(blockDevice_bootFromDevice);
    if (type == FS_TYPE_UNKNOWN) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    _supports[type].init();
    ERROR_GOTO_IF_ERROR(0);

    fs_rootFS = mm_allocate(sizeof(FS));
    if (fs_rootFS == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fs_open(fs_rootFS, blockDevice_bootFromDevice);
    ERROR_GOTO_IF_ERROR(0);

    //Begin of ext2 test code
    MajorDeviceID storageMajor = DEVICE_MAJOR_FROM_ID(blockDevice_bootFromDevice->device.id);
    MinorDeviceID storageMinor = DEVICE_INVALID_ID;
    Device* device = NULL;
    while ((device = device_iterateMinor(storageMajor, storageMinor)) != NULL) {
        if (cstring_strcmp(device->name, "HDB") == 0) {
            break;
        }
        storageMinor = DEVICE_MINOR_FROM_ID(device->id);
    }
    if (device == NULL) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    BlockDevice* ext2Device = HOST_POINTER(device, BlockDevice, device);
    _supports[FS_TYPE_EXT2].init();
    DEBUG_ASSERT_SILENT(fs_checkType(ext2Device) == FS_TYPE_EXT2);

    fs_ext2 = mm_allocate(sizeof(FS));
    if (fs_ext2 == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fs_open(fs_ext2, ext2Device);
    ERROR_GOTO_IF_ERROR(0);

    //End of ext2 test code

    _supports[FS_TYPE_DEVFS].init();
    ERROR_GOTO_IF_ERROR(0);

    fs_devFS = mm_allocate(sizeof(FS));
    if (fs_devFS == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fs_open(fs_devFS, NULL);
    ERROR_GOTO_IF_ERROR(0);

    fsIdentifier devfsMountPoint;

    FScore* rootFScore = fs_rootFS->fscore;
    vNode* rootFSrootVnode = fscore_getVnode(rootFScore, rootFScore->rootFSnode, false);  //Refer rootFScore->rootFSnode
    fsIdentifier_initStruct(&devfsMountPoint, rootFSrootVnode, "/dev", true);  //TODO: fails if dev not exist
    ERROR_GOTO_IF_ERROR(0);

    FScore* devFScore = fs_devFS->fscore;
    vNode* devFSrootVnode = fscore_getVnode(devFScore, devFScore->rootFSnode, false);
    fscore_rawMount(rootFScore, &devfsMountPoint, devFSrootVnode, EMPTY_FLAGS);
    ERROR_GOTO_IF_ERROR(0);

    fscore_releaseVnode(rootFSrootVnode);

    return;
    ERROR_FINAL_BEGIN(0);
    if (fs_rootFS != NULL) {
        mm_free(fs_rootFS);
    }

    if (fs_ext2 != NULL) {
        mm_free(fs_ext2);
    }

    if (fs_devFS != NULL) {
        mm_free(fs_devFS);
    }
}

FStype fs_checkType(BlockDevice* device) {
    for (FStype i = 0; i < FS_TYPE_NUM; ++i) {
        if (_supports[i].checkType(device)) {
            return i;
        }
    }
    return FS_TYPE_UNKNOWN;
}

void fs_open(FS* fs, BlockDevice* device) {
    FStype type = fs_checkType(device);
    _supports[type].open(fs, device);
}

void fs_close(FS* fs) {
    _supports[fs->type].close(fs);
}

File* fs_fileOpen(ConstCstring absolutePath, FCNTLopenFlags flags) {
    String absolutePathStr, dirAbsolurePath, basename;
    fsIdentifier dirIdentifier;
    fsNode* dirFSnode = NULL, * targetNode = NULL;
    FScore* finalFScore = NULL;
    vNode* dirVnode = NULL, * targetVnode = NULL;
    File* ret = NULL;

    bool isDirectory = TEST_FLAGS(flags, FCNTL_OPEN_DIRECTORY);

    string_initStructStr(&absolutePathStr, absolutePath);
    ERROR_GOTO_IF_ERROR(0);

    string_initStruct(&dirAbsolurePath);
    ERROR_GOTO_IF_ERROR(0);
    path_dirname(&absolutePathStr, &dirAbsolurePath);
    ERROR_GOTO_IF_ERROR(0);

    string_initStruct(&basename);
    ERROR_GOTO_IF_ERROR(0);
    path_basename(&absolutePathStr, &basename);
    ERROR_GOTO_IF_ERROR(0);

    FScore* rootFScore = fs_rootFS->fscore;
    vNode* rootFSrootVnode = fscore_getVnode(rootFScore, rootFScore->rootFSnode, false);    //Refer rootFSrootVnode->fsNode once
    DEBUG_ASSERT_SILENT(rootFSrootVnode != NULL);
    
    fsIdentifier_initStruct(&dirIdentifier, rootFSrootVnode, dirAbsolurePath.data, true);
    
    dirFSnode = fscore_getFSnode(rootFScore, &dirIdentifier, &finalFScore, true);   //Refer dirFSnode once

    dirVnode = fscore_getVnode(finalFScore, dirFSnode, true);
    if (dirVnode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    dirFSnode = dirVnode->fsNode;
    finalFScore = dirVnode->fscore;
    
    bool needCreate = false;
    targetNode = fsnode_lookup(dirFSnode, basename.data, isDirectory, true);    //Refer targetNode once (if found)
    if (targetNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_CHECKPOINT({
                ERROR_GOTO(0);
            }, 
            (ERROR_ID_NOT_FOUND, {
                needCreate = true;
                ERROR_CLEAR();
                break;
            })
        );
    }

    if (needCreate) {
        Timestamp timestamp;
        time_getTimestamp(&timestamp);
        vNodeAttribute attr;
        attr.createTime = timestamp.second;
        attr.lastAccessTime = timestamp.second;
        attr.lastModifyTime = timestamp.second;

        vNode_addDirectoryEntry(dirVnode, basename.data, isDirectory ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE, &attr, 0);
        ERROR_GOTO_IF_ERROR(0);

        vNode_rawReadAttr(dirVnode, &attr);
        ERROR_GOTO_IF_ERROR(0);
        attr.lastModifyTime = timestamp.second;
        vNode_rawWriteAttr(dirVnode, &attr);
        ERROR_GOTO_IF_ERROR(0);

        targetNode = fsnode_lookup(dirFSnode, basename.data, isDirectory, true);    //Refer targetNode once (if found)
        if (targetNode == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
    }

    DEBUG_ASSERT_SILENT(targetNode != NULL);

    targetVnode = fscore_getVnode(finalFScore, targetNode, false);
    
    ret = fscore_rawOpenFSentry(finalFScore, targetVnode, flags);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fscore_releaseVnode(dirVnode);
    fscore_releaseFSnode(dirFSnode);
    fsIdentifier_clearStruct(&dirIdentifier);
    fscore_releaseVnode(rootFSrootVnode);
    string_clearStruct(&basename);
    string_clearStruct(&dirAbsolurePath);
    string_clearStruct(&absolutePathStr);
    
    return ret;
    ERROR_FINAL_BEGIN(0);
    if (ret != NULL) {
        fscore_rawCloseFSentry(finalFScore, ret);
    }

    if (targetVnode != NULL) {
        fscore_releaseVnode(targetVnode);
    }

    if (dirVnode != NULL) {
        fscore_releaseVnode(dirVnode);
    }

    if (dirFSnode != NULL) {
        fscore_releaseFSnode(dirFSnode);
    }

    if (fsIdentifier_isActive(&dirIdentifier)) {
        fsIdentifier_clearStruct(&dirIdentifier);
    }

    if (string_isAvailable(&basename)) {
        string_clearStruct(&basename);
    }

    if (string_isAvailable(&dirAbsolurePath)) {
        string_clearStruct(&dirAbsolurePath);
    }

    if (string_isAvailable(&absolutePathStr)) {
        string_clearStruct(&absolutePathStr);
    }

    return NULL;
}

void fs_fileClose(File* file) {
    vNode* vnode = file->vnode;
    FScore* fscore = vnode->fscore;

    fscore_rawCloseFSentry(fscore, file);
    ERROR_GOTO_IF_ERROR(0);

    fscore_releaseVnode(vnode);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void fs_fileRead(File* file, void* buffer, Size n) {
    if (FCNTL_OPEN_EXTRACL_ACCESS_MODE(file->flags) == FCNTL_OPEN_WRITE_ONLY) {
        ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
    }

    if (file->pointer + n > file->vnode->sizeInByte) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    fsEntry_rawRead(file, buffer, n);
    ERROR_GOTO_IF_ERROR(0);
    fsEntry_rawSeek(file, file->pointer + n);

    if (TEST_FLAGS_FAIL(file->flags, FCNTL_OPEN_NOATIME)) {
        vNodeAttribute attr;
        vNode_rawReadAttr(file->vnode, &attr);
        ERROR_GOTO_IF_ERROR(0);
        
        Timestamp timestamp;
        time_getTimestamp(&timestamp);
        attr.lastAccessTime = timestamp.second;

        vNode_rawWriteAttr(file->vnode, &attr);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void fs_fileWrite(File* file, const void* buffer, Size n) {
    if (FCNTL_OPEN_EXTRACL_ACCESS_MODE(file->flags) == FCNTL_OPEN_READ_ONLY) {
        ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
    }

    if (TEST_FLAGS(file->flags, FCNTL_OPEN_APPEND)) {
        fs_fileSeek(file, 0, FS_FILE_SEEK_END);
    }

    fsEntry_rawWrite(file, buffer, n);
    ERROR_GOTO_IF_ERROR(0);
    fsEntry_rawSeek(file, file->pointer + n);

    if (TEST_FLAGS_FAIL(file->flags, FCNTL_OPEN_NOATIME)) {
        vNodeAttribute attr;
        vNode_rawReadAttr(file->vnode, &attr);
        ERROR_GOTO_IF_ERROR(0);
        
        Timestamp timestamp;
        time_getTimestamp(&timestamp);
        attr.lastAccessTime = timestamp.second;

        vNode_rawWriteAttr(file->vnode, &attr);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

Index64 fs_fileSeek(File* file, Int64 offset, Uint8 begin) {
    Index64 base = file->pointer;
    switch (begin) {
        case FS_FILE_SEEK_BEGIN:
            base = 0;
            break;
        case FS_FILE_SEEK_CURRENT:
            break;
        case FS_FILE_SEEK_END:
            base = file->vnode->sizeInByte;
            break;
        default:
            break;
    }
    base += offset;

    if ((Int64)base < 0 || base > file->vnode->sizeInByte) {
        return INVALID_INDEX64;
    }

    if (fsEntry_rawSeek(file, base) == INVALID_INDEX64) {
        return INVALID_INDEX64;
    }

    return file->pointer;
}

void fs_fileStat(File* file, FS_fileStat* stat) {
    vNode* vnode = file->vnode;
    FScore* fscore = vnode->fscore;
    
    memory_memset(stat, 0, sizeof(FS_fileStat));
    stat->deviceID = vnode->deviceID;
    stat->vnodeID = vnode->vnodeID;
    stat->nLink = 1;    //TODO: nLink not implemented actually
    Uint32 mode = file->mode;

    fsEntryType type = vnode->fsNode->entry.type;
    switch (type) {
    case FS_ENTRY_TYPE_FILE:
        FS_FILE_STAT_MODE_SET_TYPE(mode, FS_FILE_STAT_MODE_TYPE_REGULAR_FILE);
        break;
    case FS_ENTRY_TYPE_DIRECTORY:
        FS_FILE_STAT_MODE_SET_TYPE(mode, FS_FILE_STAT_MODE_TYPE_DIRECTORY);
        break;
    case FS_ENTRY_TYPE_DEVICE:
        FS_FILE_STAT_MODE_SET_TYPE(mode, FS_FILE_STAT_MODE_TYPE_BLOCK_DEVICE);
        break;
    default:
        FS_FILE_STAT_MODE_SET_TYPE(mode, 0);
        break;
    }
    stat->mode = mode;
    stat->uid = vnode->attribute.uid;  //TODO: User not implemented
    stat->gid = vnode->attribute.gid;
    if (type == FS_ENTRY_TYPE_DEVICE) {
        stat->rDevice = vnode->deviceID;
    }
    stat->size = vnode->sizeInByte;
    stat->blockSize = POWER_2(fscore->blockDevice->device.granularity);
    stat->blocks = vnode->sizeInBlock;
    stat->accessTime.second = vnode->attribute.lastAccessTime;
    stat->modifyTime.second = vnode->attribute.lastModifyTime;
    stat->createTime.second = vnode->attribute.createTime;
}