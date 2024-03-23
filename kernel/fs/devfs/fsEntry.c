#include<fs/devfs/fsEntry.h>

#include<cstring.h>
#include<devices/block/blockDevice.h>
#include<fs/devfs/blockChain.h>
#include<fs/devfs/devfs.h>
#include<fs/fsEntry.h>
#include<fs/fsStructs.h>
#include<fs/fsutil.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>

//TODO: Extremely rough code, need rework
typedef struct {
#define DEVFS_DIRECTORY_ENTRY_NAME_LIMIT    31
    char    name[DEVFS_DIRECTORY_ENTRY_NAME_LIMIT + 1];
    union {
        Device*  device;
        Range   dataRange;
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

void __devfs_fsentry_deviceToDirectoryEntry(Device* device, __DevFSdirectoryEntry* dirEntryOut);

static fsEntryOperations _DEVFSfileSystemEntryOperations = {    //Actually, only directory uses these operations, no-one knows what those device file will use
    .seek           = fsEntry_genericSeek,
    .read           = fsEntry_genericRead,
    .write          = fsEntry_genericWrite,
    .resize         = fsEntry_genericResize
};

static Result __devfs_fsEntry_readEntry(fsEntry* directory, fsEntryDesc* childDesc, Size* entrySizePtr);

static Result __devfs_fsEntry_addEntry(fsEntry* directory, fsEntryDesc* childToAdd);

static Result __devfs_fsEntry_removeEntry(fsEntry* directory, fsEntryIdentifier* childToRemove);

static Result __devfs_fsEntry_updateEntry(fsEntry* directory, fsEntryIdentifier* oldChild, fsEntryDesc* newChild);

static fsEntryDirOperations _DEVFSfileSystemEntryDirOperations = {
    .readEntry      = __devfs_fsEntry_readEntry,
    .addEntry       = __devfs_fsEntry_addEntry,
    .removeEntry    = __devfs_fsEntry_removeEntry,
    .updateEntry    = __devfs_fsEntry_updateEntry
};

Result __devfs_fsentry_directoryEntryToFSentryDesc(fsEntryIdentifier* directory, __DevFSdirectoryEntry* dirEntry, fsEntryDesc* descOut);

void __devfs_fsentry_fsEntryDescToDirectoryEntry(fsEntryDesc* descOut, __DevFSdirectoryEntry* dirEntryOut);

Result devfs_fsEntry_open(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc) {
    if (fsEntry_genericOpen(superBlock, entry, desc) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (entry->desc->identifier.type == FS_ENTRY_TYPE_DEVICE) {
        entry->operations = ((Device*)desc->device)->operations;
    } else {
        entry->operations = &_DEVFSfileSystemEntryOperations;
        if (entry->desc->identifier.type == FS_ENTRY_TYPE_DIRECTORY) {
            entry->dirOperations = &_DEVFSfileSystemEntryDirOperations;
        }
    }

    return RESULT_SUCCESS;
}

//TODO: Remove codes down

static Result __devfs_fsEntry_nullRead(fsEntry* entry, void* buffer, Size n);

static Result __devfs_fsEntry_nullWrite(fsEntry* entry, const void* buffer, Size n);

static Index64 __devfs_fsEntry_nullSeek(fsEntry* entry, Index64 seekTo);

static Result  __devfs_fsEntry_nullResize(fsEntry* entry, Size newSizeInByte);

BlockDevice _null = {
    .name = "null",
    .availableBlockNum = -1,
    .bytePerBlockShift = DEFAULT_BLOCK_SIZE_SHIFT,
    .flags = EMPTY_FLAGS,
    .parent = NULL,
    .childNum = 0,
    .blockBuffer = NULL,
    .specificInfo = OBJECT_NULL,
    .operations = NULL,
};

fsEntryOperations _nullDeviceOperations = {
    .read = __devfs_fsEntry_nullRead,
    .write = __devfs_fsEntry_nullWrite,
    .seek = __devfs_fsEntry_nullSeek,
    .resize = __devfs_fsEntry_nullResize
};

Device _nullDevice = {
    .device = &_null,
    .operations = &_nullDeviceOperations
};

static Result __devfs_fsEntry_nullRead(fsEntry* entry, void* buffer, Size n) {
    PTR_TO_VALUE(8, buffer) = 0;
    return RESULT_SUCCESS;
}

#include<print.h>
static Result __devfs_fsEntry_nullWrite(fsEntry* entry, const void* buffer, Size n) {
    printf(TERMINAL_LEVEL_OUTPUT, "%u-%s\n", n, buffer);
    return RESULT_SUCCESS;
}

static Index64 __devfs_fsEntry_nullSeek(fsEntry* entry, Index64 seekTo) {
    return 0;
}

static Result  __devfs_fsEntry_nullResize(fsEntry* entry, Size newSizeInByte) {
    return RESULT_SUCCESS;
}

//TODO: Remove codes above

Result devfs_fsEntry_buildRootDir(SuperBlock* superBlock) {
    DevFSblockChains* chains = &((DEVFSspecificInfo*)superBlock->specificInfo)->chains;
    Uint64 rootDirBegin = DEVFS_blockChain_allocChain(chains, 1);
    if (rootDirBegin == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    fsEntryDescInitArgs args = {
        .name = "",
        .parentPath = "",
        .type = FS_ENTRY_TYPE_DIRECTORY,
        .isDevice = false,
        .dataRange = RANGE(rootDirBegin << superBlock->device->bytePerBlockShift, sizeof(__DevFSdirectoryEntry)),
        .flags = EMPTY_FLAGS,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };

    fsEntryDesc_initStruct(superBlock->rootDirDesc, &args);

    Directory rootDir;
    if (fsutil_openfsEntry(superBlock, "/", FS_ENTRY_TYPE_DIRECTORY, &rootDir) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    __DevFSdirectoryEntry endDummy = {
        .dataRange = RANGE(FS_ENTRY_INVALID_POSITION, FS_ENTRY_INVALID_SIZE),
        .attribute = __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY,
        .magic = __DEVFS_DIRECTORY_ENTRY_MAGIC
    };
    memset(endDummy.name, 0, DEVFS_DIRECTORY_ENTRY_NAME_LIMIT + 1);

    fsEntry_rawSeek(&rootDir, 0);
    if (fsEntry_rawWrite(&rootDir, &endDummy, sizeof(__DevFSdirectoryEntry)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result devfs_fsEntry_initRootDir(SuperBlock* superBlock) {
    Directory rootDir;
    if (fsutil_openfsEntry(superBlock, "/", FS_ENTRY_TYPE_DIRECTORY, &rootDir) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    __DevFSdirectoryEntry deviceEntry;
    fsEntryDesc desc;
    __devfs_fsentry_deviceToDirectoryEntry(&_nullDevice, &deviceEntry);
    __devfs_fsentry_directoryEntryToFSentryDesc(&rootDir.desc->identifier, &deviceEntry, &desc);

    fsEntry_rawSeek(&rootDir, 0);
    if (fsEntry_rawDirAddEntry(&rootDir, &desc) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

void __devfs_fsentry_deviceToDirectoryEntry(Device* device, __DevFSdirectoryEntry* dirEntryOut) {
    memset(dirEntryOut, 0, sizeof(__DevFSdirectoryEntry));
    strcpy(dirEntryOut->name, device->device->name);
    dirEntryOut->device = device;
    dirEntryOut->attribute = __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE;
    dirEntryOut->magic = __DEVFS_DIRECTORY_ENTRY_MAGIC;
}

Result __devfs_fsentry_directoryEntryToFSentryDesc(fsEntryIdentifier* directory, __DevFSdirectoryEntry* dirEntry, fsEntryDesc* descOut) {
    if (dirEntry->magic != __DEVFS_DIRECTORY_ENTRY_MAGIC) {
        return RESULT_FAIL;
    }

    fsEntryDescInitArgs args;
    if (dirEntry->attribute == __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY) {
        args = (fsEntryDescInitArgs) {
            .name = "",
            .parentPath = "",
            .type = FS_ENTRY_TYPE_DUMMY,
            .isDevice = false,
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
            .isDevice = true,
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
            .isDevice = false,
            .dataRange = dirEntry->dataRange,
            .flags = EMPTY_FLAGS,
            .createTime = 0,
            .lastAccessTime = 0,
            .lastModifyTime = 0
        };
    }

    return fsEntryDesc_initStruct(descOut, &args);
}

void __devfs_fsentry_fsEntryDescToDirectoryEntry(fsEntryDesc* desc, __DevFSdirectoryEntry* dirEntryOut) {
    strcpy(dirEntryOut->name, desc->identifier.name.data);

    if (desc->identifier.type == FS_ENTRY_TYPE_DUMMY) {
        dirEntryOut->dataRange = RANGE(FS_ENTRY_INVALID_POSITION, FS_ENTRY_INVALID_SIZE);
        dirEntryOut->attribute = __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY;
    } else if (desc->identifier.type == FS_ENTRY_TYPE_DEVICE) {
        dirEntryOut->device = desc->device;
        dirEntryOut->attribute = __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE;
    } else {
        dirEntryOut->dataRange = desc->dataRange;
        dirEntryOut->attribute = desc->identifier.type == FS_ENTRY_TYPE_DIRECTORY ? __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY : __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_FILE;
    }
    
    dirEntryOut->magic = __DEVFS_DIRECTORY_ENTRY_MAGIC;
}

static Result __devfs_fsEntry_readEntry(fsEntry* directory, fsEntryDesc* childDesc, Size* entrySizePtr) {
    __DevFSdirectoryEntry dirEntry;

    if (fsEntry_rawRead(directory, &dirEntry, sizeof(__DevFSdirectoryEntry)) == RESULT_FAIL || __devfs_fsentry_directoryEntryToFSentryDesc(&directory->desc->identifier, &dirEntry, childDesc) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    *entrySizePtr = sizeof(__DevFSdirectoryEntry);
    return dirEntry.attribute == __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DUMMY ? RESULT_SUCCESS : RESULT_CONTINUE;
}

static Result __devfs_fsEntry_addEntry(fsEntry* directory, fsEntryDesc* childToAdd) {
    Size followedDataSize = directory->desc->dataRange.length - directory->pointer;
    void* followedData = kMalloc(followedDataSize);
    if (followedData == NULL || fsEntry_rawRead(directory, followedData, followedDataSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    __DevFSdirectoryEntry newEntry;
    __devfs_fsentry_fsEntryDescToDirectoryEntry(childToAdd, &newEntry);
    if (fsEntry_rawWrite(directory, &newEntry, sizeof(__DevFSdirectoryEntry)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    fsEntry_rawSeek(directory, directory->pointer + sizeof(__DevFSdirectoryEntry));
    if (fsEntry_rawWrite(directory, followedData, followedDataSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    kFree(followedData);

    return RESULT_SUCCESS;
}

static Result __devfs_fsEntry_removeEntry(fsEntry* directory, fsEntryIdentifier* childToRemove) {
    if (fsutil_lookupEntryDesc(directory, childToRemove, NULL, NULL) != RESULT_SUCCESS) {
        return RESULT_FAIL;
    }

    Index64 oldPointer = directory->pointer;
    fsEntry_rawSeek(directory, directory->pointer + sizeof(__DevFSdirectoryEntry));

    Size followedDataSize = directory->desc->dataRange.length - directory->pointer;
    void* followedData = kMalloc(followedDataSize);
    if (followedData == NULL || fsEntry_rawRead(directory, followedData, followedDataSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    fsEntry_rawSeek(directory, oldPointer);
    if (fsEntry_rawWrite(directory, followedData, followedDataSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    kFree(followedData);

    return RESULT_SUCCESS;
}

static Result __devfs_fsEntry_updateEntry(fsEntry* directory, fsEntryIdentifier* oldChild, fsEntryDesc* newChild) {
    if (fsutil_lookupEntryDesc(directory, oldChild, NULL, NULL) != RESULT_SUCCESS) {
        return RESULT_FAIL;
    }

    __DevFSdirectoryEntry updatedEntry;
    __devfs_fsentry_fsEntryDescToDirectoryEntry(newChild, &updatedEntry);
    if (fsEntry_rawWrite(directory, &updatedEntry, sizeof(__DevFSdirectoryEntry)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    fsEntry_rawSeek(directory, directory->pointer + sizeof(__DevFSdirectoryEntry));

    return RESULT_SUCCESS;
}