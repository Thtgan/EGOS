#include<fs/phospherus/phospherus_directory.h>

#include<devices/block/blockDevice.h>
#include<error.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<string.h>

typedef struct {
    ID iNodeID;
    iNodeType type;
    char reserved[52];
    char name[PHOSPHERUS_MAX_NAME_LENGTH + 1];
} __attribute__((packed)) __DirectoryEntry;

static Result __addEntry(Directory* this, ID iNodeID, iNodeType type, ConstCstring name);

static Result __removeEntry(Directory* this, Index64 entryIndex);

static Index64 __lookupEntry(Directory* this, ConstCstring name, iNodeType type);

static Result __readEntry(Directory* this, DirectoryEntry* entry, Index64 entryIndex);

DirectoryOperations directoryOperations = {
    .addEntry = __addEntry,
    .removeEntry = __removeEntry,
    .lookupEntry = __lookupEntry,
    .readEntry = __readEntry
};

static Directory* __openDirectory(iNode* iNode);

static Result __closeDirectory(Directory* directory);

static Result __doOpenDirectory(iNode* iNode, void** ret, void** directoryInMemory);

DirectoryGlobalOperations directoryGlobalOperations = {
    .openDirectory = __openDirectory,
    .closeDirectory = __closeDirectory
};

DirectoryGlobalOperations* phospherusInitDirectories() {
    return &directoryGlobalOperations;
}

static Result __addEntry(Directory* this, ID iNodeID, iNodeType type, ConstCstring name) {
    if (__lookupEntry(this, name, type) != INVALID_INDEX) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
        return RESULT_FAIL;
    }

    iNode* directoryInode = this->iNode;

    Size 
        oldBlockSize = directoryInode->onDevice.availableBlockSize,
        newBlockSize = ((this->size + 1) * sizeof(__DirectoryEntry) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    void* newDirectoryInMemory = NULL;
    if (this->size == 0) {
        newDirectoryInMemory = kMalloc(sizeof(__DirectoryEntry), MEMORY_TYPE_NORMAL);
    } else if (oldBlockSize != newBlockSize) {
        newDirectoryInMemory = kRealloc(this->directoryInMemory, newBlockSize * BLOCK_SIZE);
    }

    if (newDirectoryInMemory == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_MEMORY, ERROR_STATUS_NO_FREE_SPACE);
        return RESULT_FAIL;
    }

    this->directoryInMemory = newDirectoryInMemory;

    __DirectoryEntry* newEntry = (__DirectoryEntry*)(this->directoryInMemory + this->size * sizeof(__DirectoryEntry));
    memset(newEntry, 0, sizeof(__DirectoryEntry));
    ++this->size;

    newEntry->iNodeID = iNodeID;
    newEntry->type = type;
    strcpy(newEntry->name, name);

    if (newBlockSize != oldBlockSize && rawInodeResize(directoryInode, newBlockSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (rawInodeWriteBlocks(directoryInode, this->directoryInMemory + (newBlockSize - 1) * BLOCK_SIZE, newBlockSize - 1, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    directoryInode->onDevice.dataSize = this->size * sizeof(__DirectoryEntry);
    if (blockDeviceWriteBlocks(directoryInode->device, directoryInode->blockIndex, &directoryInode->onDevice, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __removeEntry(Directory* this, Index64 entryIndex) {
    iNode* directoryInode = this->iNode;

    Size 
        oldBlockSize = directoryInode->onDevice.availableBlockSize,
        newBlockSize = ((this->size - 1) * sizeof(__DirectoryEntry) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (this->size == 1) {
        kFree(this->directoryInMemory);
        this->directoryInMemory = NULL;
    } else {
        memmove(
            this->directoryInMemory + entryIndex * sizeof(__DirectoryEntry),
            this->directoryInMemory + (entryIndex + 1) * sizeof(__DirectoryEntry),
            (this->size - entryIndex - 1) * sizeof(__DirectoryEntry)
            );

        this->directoryInMemory = kRealloc(this->directoryInMemory, (this->size - 1) * sizeof(__DirectoryEntry));
        if (this->directoryInMemory == NULL) {
            SET_ERROR_CODE(ERROR_OBJECT_MEMORY, ERROR_STATUS_NO_FREE_SPACE);
            return RESULT_FAIL;
        }
    }
    --this->size;

    if (newBlockSize != oldBlockSize && rawInodeResize(directoryInode, newBlockSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (newBlockSize > 0 && rawInodeWriteBlocks(directoryInode, this->directoryInMemory, 0, newBlockSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    directoryInode->onDevice.dataSize = this->size * sizeof(__DirectoryEntry);
    if (blockDeviceWriteBlocks(directoryInode->device, directoryInode->blockIndex, &directoryInode->onDevice, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Index64 __lookupEntry(Directory* this, ConstCstring name, iNodeType type) {
    __DirectoryEntry* entries = this->directoryInMemory;
    for (int i = 0; i < this->size; ++i) {
        if (entries[i].type == type && strcmp(name, entries[i].name) == 0) {
            return i;
        }
    }

    SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_NOT_FOUND);
    return INVALID_INDEX;
}

static Result __readEntry(Directory* this, DirectoryEntry* entry, Index64 entryIndex) {
    if (entryIndex >= this->size) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return RESULT_FAIL;
    }

    __DirectoryEntry* diectoryEntry = ((__DirectoryEntry*)this->directoryInMemory) + entryIndex;
    entry->name = diectoryEntry->name;
    entry->iNodeID = diectoryEntry->iNodeID;
    entry->type = diectoryEntry->type;

    return RESULT_SUCCESS;
}

static Directory* __openDirectory(iNode* iNode) {
    if (iNode->onDevice.type != INODE_TYPE_DIRECTORY) {
        return NULL;
    }

    void* ret = NULL, * directoryInMemory = NULL;
    if (__doOpenDirectory(iNode, &ret, &directoryInMemory) == RESULT_FAIL) {
        if (directoryInMemory != NULL) {
            kFree(directoryInMemory);
        }

        if (ret != NULL) {
            kFree(ret);
            ret = NULL;
        }
    }

    return ret;
}

static Result __closeDirectory(Directory* directory) {
    iNode* iNode = directory->iNode;

    directory->iNode = NULL;
    kFree(directory->directoryInMemory);
    kFree(directory);

    return RESULT_SUCCESS;
}

static Result __doOpenDirectory(iNode* iNode, void** retPtr, void** directoryInMemoryPtr) {
    Directory* ret = NULL, * directoryInMemory = NULL;

    *retPtr = ret = kMalloc(sizeof(Directory), MEMORY_TYPE_NORMAL);
    if (ret == NULL) {
        return RESULT_FAIL;
    }

    ret->iNode = iNode;
    ret->size = iNode->onDevice.dataSize / sizeof(__DirectoryEntry);
    ret->operations = &directoryOperations;
    ret->directoryInMemory = NULL;
    if (ret->size > 0) {
        *directoryInMemoryPtr = directoryInMemory = ret->directoryInMemory = kMalloc(iNode->onDevice.availableBlockSize * BLOCK_SIZE, MEMORY_TYPE_NORMAL);
        if (directoryInMemory == NULL) {
            return RESULT_FAIL;
        }

        if (rawInodeReadBlocks(iNode, ret->directoryInMemory, 0, iNode->onDevice.availableBlockSize) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return RESULT_SUCCESS;
}