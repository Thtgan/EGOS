#include<fs/devfs/fsEntry.h>

#include<cstring.h>
#include<devices/device.h>
#include<devices/block/blockDevice.h>
#include<fs/devfs/blockChain.h>
#include<fs/devfs/devfs.h>
#include<fs/fcntl.h>
#include<fs/fsEntry.h>
#include<fs/fsutil.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<time/time.h>
#include<error.h>

//TODO: Extremely rough code, need rework
typedef struct __DevFSdirectoryEntry {
#define DEVFS_DIRECTORY_ENTRY_NAME_LIMIT    31
    char    name[DEVFS_DIRECTORY_ENTRY_NAME_LIMIT + 1];
    union {
        Range       dataRange;
        DeviceID    device;
    };
    Uint8   attribute;
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY FLAG8(0)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN    FLAG8(1)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY FLAG8(2)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_FILE      FLAG8(3)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE    FLAG8(4)
#define __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY     -1
    Uint32  magic;
#define __DEVFS_DIRECTORY_ENTRY_MAGIC   0xDE7F53A6
    Uint8 padding[8];
} __DevFSdirectoryEntry;

static void __devfs_fsEntry_deviceToDirectoryEntry(Device* device, __DevFSdirectoryEntry* dirEntryOut);

static fsEntryOperations _devfs_fsEntryOperations = {   //Actually, only directory uses these operations, no-one knows what those device file will use
    .seek           = fsEntry_genericSeek,
    .read           = fsEntry_genericRead,
    .write          = fsEntry_genericWrite,
    .resize         = fsEntry_genericResize
};

static bool __devfs_fsEntry_readEntry(fsEntry* directory, fsEntryDesc* childDesc, Size* entrySizePtr);

static void __devfs_fsEntry_addEntry(fsEntry* directory, fsEntryDesc* childToAdd);

static void __devfs_fsEntry_removeEntry(fsEntry* directory, fsEntryIdentifier* childToRemove);

static void __devfs_fsEntry_updateEntry(fsEntry* directory, fsEntryIdentifier* oldChild, fsEntryDesc* newChild);

static fsEntryDirOperations _devfs_fsEntryDirOperations = {
    .readEntry      = __devfs_fsEntry_readEntry,
    .addEntry       = __devfs_fsEntry_addEntry,
    .removeEntry    = __devfs_fsEntry_removeEntry,
    .updateEntry    = __devfs_fsEntry_updateEntry
};

static void __devfs_fsentry_directoryEntryToFSentryDesc(fsEntryIdentifier* directory, __DevFSdirectoryEntry* dirEntry, fsEntryDesc* descOut);

static void __devfs_fsEntry_fsEntryDescToDirectoryEntry(fsEntryDesc* descOut, __DevFSdirectoryEntry* dirEntryOut);

void devfs_fsEntry_open(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags) {
    fsEntry_genericOpen(superBlock, entry, desc, flags);
    ERROR_GOTO_IF_ERROR(0);

    entry->operations = &_devfs_fsEntryOperations;
    // if (entry->desc->type == FS_ENTRY_TYPE_DEVICE) {
    //     Device* device = device_getDevice(desc->device);
    //     if (device == NULL) {
    //         ERROR_ASSERT_ANY();
    //         ERROR_GOTO(0);
    //     }
    // } else {
    //     if (entry->desc->type == FS_ENTRY_TYPE_DIRECTORY) {
    //         entry->dirOperations = &_devfs_fsEntryDirOperations;
    //     }
    // }
    if (entry->desc->type == FS_ENTRY_TYPE_DIRECTORY) {
        entry->dirOperations = &_devfs_fsEntryDirOperations;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void devfs_fsEntry_create(SuperBlock* superBlock, fsEntryDesc* descOut, ConstCstring name, ConstCstring parentPath, fsEntryType type, DeviceID deviceID, Flags16 flags) {
    Uint8 newBlock = INVALID_INDEX;

    DevFSblockChains* chains = &((DEVFSspecificInfo*)superBlock->specificInfo)->chains;
    newBlock = devfs_blockChain_allocChain(chains, 1);
    if (newBlock == INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Device* device = &superBlock->blockDevice->device;
    Timestamp timestamp;
    time_getTimestamp(&timestamp);
    fsEntryDescInitArgs args = {
        .name           = name,
        .parentPath     = parentPath,
        .type           = type,
        .flags          = flags,
        .createTime     = timestamp.second,
        .lastAccessTime = timestamp.second,
        .lastModifyTime = timestamp.second
    };

    if (type == FS_ENTRY_TYPE_DEVICE) {
        args.device     = deviceID;
    } else {
        args.dataRange  = RANGE((Uint64)newBlock * POWER_2(device->granularity), 0);
    }

    fsEntryDesc_initStruct(descOut, &args);

    return;
    ERROR_FINAL_BEGIN(0);
    if (newBlock != INVALID_INDEX) {
        devfs_blockChain_freeChain(chains, newBlock);
    }
}

void devfs_fsEntry_buildRootDir(SuperBlock* superBlock) {
    Uint8 rootDirBegin = INVALID_INDEX;

    DevFSblockChains* chains = &((DEVFSspecificInfo*)superBlock->specificInfo)->chains;
    rootDirBegin = devfs_blockChain_allocChain(chains, 1);
    if (rootDirBegin == INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Device* device = &superBlock->blockDevice->device;
    fsEntryDescInitArgs args = {
        .name = "",
        .parentPath = "",
        .type = FS_ENTRY_TYPE_DIRECTORY,
        .dataRange = RANGE(rootDirBegin * POWER_2(device->granularity), sizeof(__DevFSdirectoryEntry)),
        .flags = EMPTY_FLAGS,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };

    fsEntryDesc_initStruct(superBlock->rootDirDesc, &args);

    Directory rootDir;  //Not closing to keep iNode in buffer
    fsutil_openfsEntry(superBlock, "/", &rootDir, FCNTL_OPEN_DIRECTORY_DEFAULT_FLAGS);
    ERROR_GOTO_IF_ERROR(0); //TODO: Close rootDir if failed

    __DevFSdirectoryEntry endDummy = {
        .dataRange = RANGE(FS_ENTRY_INVALID_POSITION, FS_ENTRY_INVALID_SIZE),
        .attribute = __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY,
        .magic = __DEVFS_DIRECTORY_ENTRY_MAGIC
    };
    memory_memset(endDummy.name, 0, DEVFS_DIRECTORY_ENTRY_NAME_LIMIT + 1);

    fsEntry_rawSeek(&rootDir, 0);
    
    fsEntry_rawWrite(&rootDir, &endDummy, sizeof(__DevFSdirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
    if (rootDirBegin != INVALID_INDEX) {
        devfs_blockChain_freeChain(chains, rootDirBegin);
    }
}

void devfs_fsEntry_initRootDir(SuperBlock* superBlock) {
    Directory rootDir;
    fsutil_openfsEntry(superBlock, "/", &rootDir, FCNTL_OPEN_DIRECTORY_DEFAULT_FLAGS);
    ERROR_GOTO_IF_ERROR(0); //TODO: Close rootDir if failed

    __DevFSdirectoryEntry deviceEntry;
    fsEntryDesc desc;
    for (MajorDeviceID major = device_iterateMajor(DEVICE_INVALID_ID); major != DEVICE_INVALID_ID; major = device_iterateMajor(major)) {    //TODO: What if device joins after boot?
        for (Device* device = device_iterateMinor(major, DEVICE_INVALID_ID); device != NULL; device = device_iterateMinor(major, DEVICE_MINOR_FROM_ID(device->id))) {
            __devfs_fsEntry_deviceToDirectoryEntry(device, &deviceEntry);
            __devfs_fsentry_directoryEntryToFSentryDesc(&rootDir.desc->identifier, &deviceEntry, &desc);
            ERROR_GOTO_IF_ERROR(0);
            fsEntry_rawSeek(&rootDir, 0);

            fsEntry_rawDirAddEntry(&rootDir, &desc);
            ERROR_GOTO_IF_ERROR(0);
            ERROR_CHECKPOINT();
        }
    }

    fsutil_closefsEntry(&rootDir);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __devfs_fsEntry_deviceToDirectoryEntry(Device* device, __DevFSdirectoryEntry* dirEntryOut) {
    memory_memset(dirEntryOut, 0, sizeof(__DevFSdirectoryEntry));
    cstring_strcpy(dirEntryOut->name, device->name);
    dirEntryOut->device     = device->id;
    dirEntryOut->attribute  = __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE;
    dirEntryOut->magic      = __DEVFS_DIRECTORY_ENTRY_MAGIC;
}

static void __devfs_fsentry_directoryEntryToFSentryDesc(fsEntryIdentifier* directory, __DevFSdirectoryEntry* dirEntry, fsEntryDesc* descOut) {
    if (dirEntry->magic != __DEVFS_DIRECTORY_ENTRY_MAGIC) {
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    fsEntryDescInitArgs args;
    if (dirEntry->attribute == __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY) {
        args = (fsEntryDescInitArgs) {
            .name = "",
            .parentPath = "",
            .type = FS_ENTRY_TYPE_DUMMY,
            .dataRange = RANGE(FS_ENTRY_INVALID_POSITION, FS_ENTRY_INVALID_SIZE),
            .flags = EMPTY_FLAGS,
            .createTime = 0,
            .lastAccessTime = 0,
            .lastModifyTime = 0
        };
    } else if (TEST_FLAGS(dirEntry->attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE)) {
        args = (fsEntryDescInitArgs) {
            .name = dirEntry->name,
            .parentPath = directory->parentPath.data,
            .type = FS_ENTRY_TYPE_DEVICE,
            .device = dirEntry->device,
            .flags = EMPTY_FLAGS,
            .createTime = 0,
            .lastAccessTime = 0,
            .lastModifyTime = 0
        };
    } else {
        args = (fsEntryDescInitArgs) {
            .name = dirEntry->name,
            .parentPath = directory->parentPath.data,
            .type = TEST_FLAGS(dirEntry->attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY) ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE,
            .dataRange = dirEntry->dataRange,
            .flags = EMPTY_FLAGS,
            .createTime = 0,
            .lastAccessTime = 0,
            .lastModifyTime = 0
        };
    }

    fsEntryDesc_initStruct(descOut, &args);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __devfs_fsEntry_fsEntryDescToDirectoryEntry(fsEntryDesc* desc, __DevFSdirectoryEntry* dirEntryOut) {
    cstring_strcpy(dirEntryOut->name, desc->identifier.name.data);

    if (desc->type == FS_ENTRY_TYPE_DUMMY) {
        dirEntryOut->dataRange = RANGE(FS_ENTRY_INVALID_POSITION, FS_ENTRY_INVALID_SIZE);
        dirEntryOut->attribute = __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY;
    } else if (desc->type == FS_ENTRY_TYPE_DEVICE) {
        dirEntryOut->device = desc->device;
        dirEntryOut->attribute = __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE;
    } else {
        dirEntryOut->dataRange = desc->dataRange;
        dirEntryOut->attribute = desc->type == FS_ENTRY_TYPE_DIRECTORY ? __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY : __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_FILE;
    }
    
    dirEntryOut->magic = __DEVFS_DIRECTORY_ENTRY_MAGIC;
}

static bool __devfs_fsEntry_readEntry(fsEntry* directory, fsEntryDesc* childDesc, Size* entrySizePtr) {
    __DevFSdirectoryEntry dirEntry;

    fsEntry_rawRead(directory, &dirEntry, sizeof(__DevFSdirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    __devfs_fsentry_directoryEntryToFSentryDesc(&directory->desc->identifier, &dirEntry, childDesc);
    ERROR_GOTO_IF_ERROR(0);

    *entrySizePtr = sizeof(__DevFSdirectoryEntry);
    return dirEntry.attribute == __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY;

    ERROR_FINAL_BEGIN(0);
    return false;
}

static void __devfs_fsEntry_addEntry(fsEntry* directory, fsEntryDesc* childToAdd) {
    void* followedData = NULL;
    Size followedDataSize = directory->desc->dataRange.length - directory->pointer;
    
    followedData = memory_allocate(followedDataSize);
    if (followedData == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fsEntry_rawRead(directory, followedData, followedDataSize);
    ERROR_GOTO_IF_ERROR(0);

    __DevFSdirectoryEntry newEntry;
    __devfs_fsEntry_fsEntryDescToDirectoryEntry(childToAdd, &newEntry);

    fsEntry_rawWrite(directory, &newEntry, sizeof(__DevFSdirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    fsEntry_rawSeek(directory, directory->pointer + sizeof(__DevFSdirectoryEntry));
    fsEntry_rawWrite(directory, followedData, followedDataSize);
    ERROR_GOTO_IF_ERROR(0);

    memory_free(followedData);
    followedData = NULL;

    return;
    ERROR_FINAL_BEGIN(0);
    if (followedData != NULL) {
        memory_free(followedData);
    }
}

static void __devfs_fsEntry_removeEntry(fsEntry* directory, fsEntryIdentifier* childToRemove) {
    void* followedData = NULL;
    fsutil_lookupEntryDesc(directory, childToRemove->name.data, childToRemove->isDirectory, NULL, NULL);
    ERROR_GOTO_IF_ERROR(0);

    Index64 oldPointer = directory->pointer;
    fsEntry_rawSeek(directory, directory->pointer + sizeof(__DevFSdirectoryEntry));

    Size followedDataSize = directory->desc->dataRange.length - directory->pointer;
    followedData = memory_allocate(followedDataSize);
    if (followedData == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fsEntry_rawRead(directory, followedData, followedDataSize);
    ERROR_GOTO_IF_ERROR(0);

    fsEntry_rawSeek(directory, oldPointer);
    fsEntry_rawWrite(directory, followedData, followedDataSize);
    ERROR_GOTO_IF_ERROR(0);

    memory_free(followedData);
    followedData = NULL;

    return;
    ERROR_FINAL_BEGIN(0);
    if (followedData != NULL) {
        memory_free(followedData);
    }
}

static void __devfs_fsEntry_updateEntry(fsEntry* directory, fsEntryIdentifier* oldChild, fsEntryDesc* newChild) {
    fsutil_lookupEntryDesc(directory, oldChild->name.data, oldChild->isDirectory, NULL, NULL);
    ERROR_GOTO_IF_ERROR(0);

    __DevFSdirectoryEntry updatedEntry;
    __devfs_fsEntry_fsEntryDescToDirectoryEntry(newChild, &updatedEntry);

    fsEntry_rawWrite(directory, &updatedEntry, sizeof(__DevFSdirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    fsEntry_rawSeek(directory, directory->pointer + sizeof(__DevFSdirectoryEntry));

    return;
    ERROR_FINAL_BEGIN(0);
}