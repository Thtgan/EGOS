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
#include<lib/errorPosix.h>

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
    ERROR_THROW_NEW_IF(blockDevice_bootFromDevice == NULL, ERROR_INVALID_STATE, error_out);

    FStype type = fs_checkType(blockDevice_bootFromDevice);
    ERROR_THROW_NEW_IF(type == FS_TYPE_UNKNOWN, ERROR_INVALID_STATE, error_out);

    _supports[type].init();
    CHECK_ERROR(error_out);

    fs_rootFS = mm_allocate(sizeof(FS));
    ERROR_THROW_NEW_IF(fs_rootFS == NULL, ERROR_OUT_OF_MEMORY, error_out);

    fs_open(fs_rootFS, blockDevice_bootFromDevice);
    CHECK_ERROR(error_out);

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
    ERROR_THROW_NEW_IF(device == NULL, ERROR_NOT_FOUND, error_out);

    BlockDevice* ext2Device = HOST_POINTER(device, BlockDevice, device);
    _supports[FS_TYPE_EXT2].init();
    DEBUG_ASSERT_SILENT(fs_checkType(ext2Device) == FS_TYPE_EXT2);

    fs_ext2 = mm_allocate(sizeof(FS));
    ERROR_THROW_NEW_IF(fs_ext2 == NULL, ERROR_OUT_OF_MEMORY, error_out);

    fs_open(fs_ext2, ext2Device);
    CHECK_ERROR(error_out);

    //End of ext2 test code

    _supports[FS_TYPE_DEVFS].init();
    CHECK_ERROR(error_out);

    fs_devFS = mm_allocate(sizeof(FS));
    ERROR_THROW_NEW_IF(fs_devFS == NULL, ERROR_OUT_OF_MEMORY, error_out);

    fs_open(fs_devFS, NULL);
    CHECK_ERROR(error_out);

    fsIdentifier devfsMountPoint;
    fsIdentifier ext2MountPoint;

    FScore* rootFScore = fs_rootFS->fscore;
    vNode* rootFSrootVnode = fscore_getVnode(rootFScore, rootFScore->rootFSnode, false);  //Refer rootFScore->rootFSnode
    fsIdentifier_initStruct(&devfsMountPoint, rootFSrootVnode, "/dev", true);   //TODO: fails if dev not exist
    CHECK_ERROR(error_out);
    fsIdentifier_initStruct(&ext2MountPoint, rootFSrootVnode, "/mnt", true);    //TODO: fails if mnt not exist
    CHECK_ERROR(error_out);

    FScore* devFScore = fs_devFS->fscore;
    vNode* devFSrootVnode = fscore_getVnode(devFScore, devFScore->rootFSnode, false);
    fscore_rawMount(rootFScore, &devfsMountPoint, devFSrootVnode, EMPTY_FLAGS);
    CHECK_ERROR(error_out);
    
    FScore* ext2FScore = fs_ext2->fscore;
    vNode* ext2rootVnode = fscore_getVnode(ext2FScore, ext2FScore->rootFSnode, false);
    fscore_rawMount(rootFScore, &ext2MountPoint, ext2rootVnode, EMPTY_FLAGS);
    CHECK_ERROR(error_out);

    fscore_releaseVnode(rootFSrootVnode);
    fscore_releaseVnode(devFSrootVnode);
    fscore_releaseVnode(ext2rootVnode);

    return;
error_out:
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
    CHECK_ERROR(error_out);

    string_initStruct(&dirAbsolurePath);
    CHECK_ERROR(error_out);
    path_dirname(&absolutePathStr, &dirAbsolurePath);
    CHECK_ERROR(error_out);

    string_initStruct(&basename);
    CHECK_ERROR(error_out);
    path_basename(&absolutePathStr, &basename);
    CHECK_ERROR(error_out);

    FScore* rootFScore = fs_rootFS->fscore;
    vNode* rootFSrootVnode = fscore_getVnode(rootFScore, rootFScore->rootFSnode, false);    //Refer rootFSrootVnode->fsNode once
    DEBUG_ASSERT_SILENT(rootFSrootVnode != NULL);
    
    fsIdentifier_initStruct(&dirIdentifier, rootFSrootVnode, dirAbsolurePath.data, true);
    
    dirFSnode = fscore_getFSnode(rootFScore, &dirIdentifier, &finalFScore, true);   //Refer dirFSnode once

    dirVnode = fscore_getVnode(finalFScore, dirFSnode, true);
    ERROR_THROW_NEW_IF(dirVnode == NULL, ERROR_OUT_OF_MEMORY, error_out);

    if (dirFSnode != dirVnode->fsNode) {    //The fsNode we found is mounted, transfer pointer and refer count to actual one
        fsnode_derefer(dirFSnode);
        dirFSnode = dirVnode->fsNode;
        fsnode_refer(dirFSnode);
    }

    finalFScore = dirVnode->fscore;
    
    bool needCreate = false;
    targetNode = fsnode_lookup(dirFSnode, basename.data, isDirectory, true);    //Refer targetNode once (if found)
    if (targetNode == NULL) {
        ErrorCode err = error_get_code();
        if (err == ERROR_NOT_FOUND) {
            needCreate = true;
        } else {
            goto error_out;
        }
    }

    if (needCreate) {
        Timestamp timestamp;
        time_getTimestamp(&timestamp);
        FSnodeAttribute attr;
        attr.createTime = timestamp.second;
        attr.lastAccessTime = timestamp.second;
        attr.lastModifyTime = timestamp.second;

        DirectoryEntry newEntry = (DirectoryEntry) {
            .name = basename.data,
            .type = isDirectory ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE,
            .mode = 0,  //TODO: mode not used yet
            .vnodeID = DIRECTORY_ENTRY_VNDOE_ID_ANY,
            .size = DIRECTORY_ENTRY_SIZE_ANY,
            .pointsTo = DIRECTORY_ENTRY_POINTS_TO_ANY
        };

        vNode_addDirectoryEntry(dirVnode, &newEntry, &attr);
        CHECK_ERROR(error_out);

        dirVnode->fsNode->attribute.lastModifyTime = timestamp.second;  //TODO: Write this back to directory data

        targetNode = fsnode_lookup(dirFSnode, basename.data, isDirectory, true);    //Refer targetNode once (if found)
        ERROR_THROW_NEW_IF(targetNode == NULL, ERROR_NOT_FOUND, error_out);
    }

    DEBUG_ASSERT_SILENT(targetNode != NULL);

    targetVnode = fscore_getVnode(finalFScore, targetNode, false);
    
    ret = fscore_rawOpenFSentry(finalFScore, targetVnode, flags);
    ERROR_THROW_NEW_IF(ret == NULL, ERROR_OUT_OF_MEMORY, error_out);

    fscore_releaseVnode(dirVnode);
    fscore_releaseFSnode(dirFSnode);
    fsIdentifier_clearStruct(&dirIdentifier);
    fscore_releaseVnode(rootFSrootVnode);
    string_clearStruct(&basename);
    string_clearStruct(&dirAbsolurePath);
    string_clearStruct(&absolutePathStr);
    
    return ret;
error_out:
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
    CHECK_ERROR(error_out);

    fscore_releaseVnode(vnode);
    CHECK_ERROR(error_out);

    return;
error_out:
}

void fs_fileRead(File* file, void* buffer, Size n) {
    ERROR_THROW_NEW_IF(FCNTL_OPEN_EXTRACL_ACCESS_MODE(file->flags) == FCNTL_OPEN_WRITE_ONLY, ERROR_PERMISSION_DENIED, error_out);

    ERROR_THROW_NEW_IF(file->pointer + n > file->vnode->size, ERROR_INVALID_ARGUMENT, error_out);

    fsEntry_rawRead(file, buffer, n);
    CHECK_ERROR(error_out);
    fsEntry_rawSeek(file, file->pointer + n);

    if (TEST_FLAGS_FAIL(file->flags, FCNTL_OPEN_NOATIME)) {
        Timestamp timestamp;
        time_getTimestamp(&timestamp);
        file->vnode->fsNode->attribute.lastAccessTime = timestamp.second;   //TODO: Write this back to directory data
    }

    return;
error_out:
}

void fs_fileWrite(File* file, const void* buffer, Size n) {
    ERROR_THROW_NEW_IF(FCNTL_OPEN_EXTRACL_ACCESS_MODE(file->flags) == FCNTL_OPEN_READ_ONLY, ERROR_PERMISSION_DENIED, error_out);

    if (TEST_FLAGS(file->flags, FCNTL_OPEN_APPEND)) {
        fs_fileSeek(file, 0, FS_FILE_SEEK_END);
    }

    fsEntry_rawWrite(file, buffer, n);
    CHECK_ERROR(error_out);
    fsEntry_rawSeek(file, file->pointer + n);

    if (TEST_FLAGS_FAIL(file->flags, FCNTL_OPEN_NOATIME)) {
        Timestamp timestamp;
        time_getTimestamp(&timestamp);
        file->vnode->fsNode->attribute.lastAccessTime = timestamp.second;   //TODO: Write this back to directory data
    }

    return;
error_out:
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
            base = file->vnode->size;
            break;
        default:
            break;
    }
    base += offset;

    if ((Int64)base < 0 || base > file->vnode->size) {
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
    FSnodeAttribute* attribute = &vnode->fsNode->attribute;
    stat->uid = attribute->uid;  //TODO: User not implemented
    stat->gid = attribute->gid;
    if (type == FS_ENTRY_TYPE_DEVICE) {
        stat->rDevice = vnode->deviceID;
    }
    stat->size = vnode->size;
    stat->blockSize = POWER_2(fscore->blockDevice->device.granularity);
    stat->blocks = vnode->tokenSpaceSize / stat->blockSize;
    stat->accessTime.second = attribute->lastAccessTime;
    stat->modifyTime.second = attribute->lastModifyTime;
    stat->createTime.second = attribute->createTime;
}
