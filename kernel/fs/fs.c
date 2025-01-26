#include<fs/fs.h>

#include<devices/block/blockDevice.h>
#include<devices/block/memoryBlockDevice.h>
#include<devices/terminal/terminalSwitch.h>
#include<fs/devfs/devfs.h>
#include<fs/fat32/fat32.h>
#include<fs/fsEntry.h>
#include<fs/fsutil.h>
#include<kernel.h>
#include<kit/util.h>
#include<memory/paging.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<error.h>

FS* rootFS = NULL, * devFS = NULL;
BlockDevice devfsBlockDevice;

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
    void* region = NULL;

    if (firstBootablePartition == NULL) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    FStype type = fs_checkType(firstBootablePartition);
    if (type == FS_TYPE_UNKNOWN) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    _supports[type].init();
    ERROR_GOTO_IF_ERROR(0);

    rootFS = memory_allocate(sizeof(FS));
    if (rootFS == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fs_open(rootFS, firstBootablePartition);
    ERROR_GOTO_IF_ERROR(0);
    
    region = memory_allocateFrame(DEVFS_BLOCKDEVICE_BLOCK_NUM * BLOCK_DEVICE_DEFAULT_BLOCK_SIZE / PAGE_SIZE);
    if (region == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    region = paging_convertAddressP2V(region);

    memoryBlockDevice_initStruct(&devfsBlockDevice, region, DEVFS_BLOCKDEVICE_BLOCK_NUM * BLOCK_DEVICE_DEFAULT_BLOCK_SIZE, "DEVFS_BLKDEVICE");
    ERROR_GOTO_IF_ERROR(0);

    _supports[FS_TYPE_DEVFS].init();
    ERROR_GOTO_IF_ERROR(0);

    devFS = memory_allocate(sizeof(FS));
    if (devFS == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fs_open(devFS, &devfsBlockDevice);
    ERROR_GOTO_IF_ERROR(0);

    fsEntryIdentifier devMountIdentifier;
    fsEntryIdentifier_initStruct(&devMountIdentifier, "/dev", FS_ENTRY_TYPE_DIRECTORY);
    ERROR_GOTO_IF_ERROR(0);

    superBlock_rawMount(rootFS->superBlock, &devMountIdentifier, devFS->superBlock, devFS->superBlock->rootDirDesc);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
    if (rootFS != NULL) {
        memory_free(rootFS);
    }

    if (region != NULL) {
        memory_freeFrame(paging_convertAddressV2P(region));
    }

    if (devFS != NULL) {
        memory_free(devFS);
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

void fs_fileRead(File* file, void* buffer, Size n) {
    fsutil_fileRead(file, buffer, n);
}

void fs_fileWrite(File* file, const void* buffer, Size n) {
    fsutil_fileWrite(file, buffer, n);
}

Index64 fs_fileSeek(File* file, Int64 offset, Uint8 begin) {
    return fsutil_fileSeek(file, offset, begin);
}

void fs_fileOpen(File* file, ConstCstring filepath, FCNTLopenFlags flags) {
    fsutil_openfsEntry(rootFS->superBlock, filepath, file, flags);
}

void fs_fileClose(File* file) {
    fsutil_closefsEntry(file);
}

void fs_fileStat(File* file, FS_fileStat* stat) {
    fsEntryDesc* desc = file->desc;
    iNode* iNode = file->iNode;
    SuperBlock* superBlock = iNode->superBlock;
    
    memory_memset(stat, 0, sizeof(FS_fileStat));
    stat->deviceID = iNode->device->id;
    stat->iNodeID = iNode->iNodeID;
    stat->nLink = 1;    //TODO: nLink not implemented actually
    Uint32 mode = desc->mode;
    switch (desc->type) {
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
    stat->uid = 0;  //TODO: User not implemented
    stat->gid = 0;
    if (desc->type == FS_ENTRY_TYPE_DEVICE) {
        stat->rDevice = iNode->device->id;
    }
    stat->size = desc->dataRange.length;
    stat->blockSize = POWER_2(superBlock->blockDevice->device.granularity);
    stat->blocks = iNode->sizeInBlock;
    stat->accessTime.second = desc->lastAccessTime;
    stat->modifyTime.second = desc->lastModifyTime;
    stat->createTime.second = desc->createTime;
}