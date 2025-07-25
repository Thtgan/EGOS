#include<fs/fs.h>

#include<devices/blockDevice.h>
#include<devices/memoryBlockDevice.h>
#include<fs/devfs/devfs.h>
#include<fs/fat32/fat32.h>
#include<fs/fsEntry.h>
#include<fs/fsIdentifier.h>
#include<fs/locate.h>
#include<fs/path.h>
#include<kit/util.h>
#include<memory/paging.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<structs/hashTable.h>
#include<error.h>

FS* fs_rootFS = NULL, * fs_devFS = NULL;

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
    fsIdentifier_initStruct(&devfsMountPoint, fs_rootFS->fsCore->rootVnode, "/dev", true);  //TODO: fails if dev not exist
    ERROR_GOTO_IF_ERROR(0);

    fsCore_rawMount(fs_rootFS->fsCore, &devfsMountPoint, fs_devFS->fsCore->rootVnode, EMPTY_FLAGS);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
    if (fs_rootFS != NULL) {
        mm_free(fs_rootFS);
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

File* fs_fileOpen(ConstCstring path, FCNTLopenFlags flags) {
    fsIdentifier identifier;
    FScore* fsCore = NULL;
    fsNode* parentDirNode = NULL;
    vNode* vnode = NULL, * parentDirVnode = NULL;
    File* ret = NULL;
    String basename;
    
    bool isDirectory = TEST_FLAGS(flags, FCNTL_OPEN_DIRECTORY);
    fsIdentifier_initStruct(&identifier, fs_rootFS->fsCore->rootVnode, path, isDirectory);
    ERROR_GOTO_IF_ERROR(0);
    parentDirNode = locate(&identifier, flags, &parentDirVnode, &fsCore);    //Refer 'parentDirNode' once (if found), refer 'parentDirVnode->fsNode' once (if vNode opened)
    bool needCreate = false;
    if (parentDirNode == NULL) {
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
    DEBUG_ASSERT_SILENT(fsCore != NULL);

    if (needCreate) {
        DEBUG_ASSERT_SILENT(parentDirNode == NULL);
        DEBUG_ASSERT_SILENT(parentDirVnode != NULL);
        if (TEST_FLAGS_FAIL(flags, FCNTL_OPEN_CREAT)) {
            ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
        }

        path_basename(&identifier.path, &basename);
        ERROR_GOTO_IF_ERROR(0);

        Timestamp timestamp;
        time_getTimestamp(&timestamp);
        vNodeAttribute attr;
        attr.createTime = timestamp.second;
        attr.lastAccessTime = timestamp.second;
        attr.lastModifyTime = timestamp.second;

        vNode_rawAddDirectoryEntry(parentDirVnode, basename.data, isDirectory ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE, &attr, 0);
        ERROR_GOTO_IF_ERROR(0);
        parentDirNode = vNode_lookupDirectoryEntry(parentDirVnode, basename.data, isDirectory);  //Refer 'parentDirNode' once
        ERROR_GOTO_IF_ERROR(0);
        
        vNode_rawReadAttr(parentDirVnode, &attr);
        ERROR_GOTO_IF_ERROR(0);
        attr.lastModifyTime = timestamp.second;
        vNode_rawWriteAttr(parentDirVnode, &attr);
        ERROR_GOTO_IF_ERROR(0);
        
        string_clearStruct(&basename);
    } else {
        if (isDirectory && parentDirNode->type != FS_ENTRY_TYPE_DIRECTORY) {
            ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
        }
    }
    DEBUG_ASSERT_SILENT(parentDirNode != NULL);

    if (parentDirVnode != NULL) {
        fsCore_closeVnode(parentDirVnode);  //Release 'parentDirVnode->fsNode' once (if vNode opened in locate)
    }
    
    vnode = fsNode_getVnode(parentDirNode, fsCore); //Refer 'parentDirNode' once (if vNode not opened)
    ERROR_GOTO_IF_ERROR(0);
    fsNode_release(parentDirNode);  //Release 'parentDirNode' once (from locate or vNode_lookupDirectoryEntry)
    parentDirNode = NULL;

    ret = fsCore_rawOpenFSentry(fsCore, vnode, flags);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fsIdentifier_clearStruct(&identifier);

    return ret;
    ERROR_FINAL_BEGIN(0);

    ErrorRecord tmp;
    error_readRecord(&tmp);
    ERROR_CLEAR();
    
    if (string_isAvailable(&basename)) {
        string_clearStruct(&basename);
    }

    if (ret != NULL) {
        DEBUG_ASSERT_SILENT(fsCore != NULL);
        fsCore_rawCloseFSentry(fsCore, ret);
        ERROR_ASSERT_NONE();
    }

    if (parentDirNode != NULL) {
        fsNode_release(parentDirNode); //Release 'parentDirNode' once (from locate or vNode_lookupDirectoryEntry)
    }

    if (vnode != NULL) {
        fsCore_closeVnode(vnode);   //Release 'vnode->fsNode' (parentDirNode) once (if vNode opened)
        ERROR_ASSERT_NONE();
    }
    
    if (parentDirVnode != NULL) {
        fsCore_closeVnode(parentDirVnode);  //Release 'parentDirVnode->fsNode' once (if vNode opened in locate)
        ERROR_ASSERT_NONE();
    }

    if (fsIdentifier_isActive(&identifier)) {
        fsIdentifier_clearStruct(&identifier);
        ERROR_ASSERT_NONE();
    }

    error_writeRecord(&tmp);

    return NULL;
}

void fs_fileClose(File* file) {
    vNode* vnode = file->vnode;
    FScore* fsCore = vnode->fsCore;

    fsCore_rawCloseFSentry(fsCore, file);
    ERROR_GOTO_IF_ERROR(0);

    fsCore_rawCloseVnode(fsCore, vnode);
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
    FScore* fsCore = vnode->fsCore;
    
    memory_memset(stat, 0, sizeof(FS_fileStat));
    stat->deviceID = vnode->deviceID;
    stat->vnodeID = vnode->vnodeID;
    stat->nLink = 1;    //TODO: nLink not implemented actually
    Uint32 mode = file->mode;

    fsEntryType type = vnode->fsNode->type;
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
    stat->blockSize = POWER_2(fsCore->blockDevice->device.granularity);
    stat->blocks = vnode->sizeInBlock;
    stat->accessTime.second = vnode->attribute.lastAccessTime;
    stat->modifyTime.second = vnode->attribute.lastModifyTime;
    stat->createTime.second = vnode->attribute.createTime;
}