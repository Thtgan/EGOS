#include<fs/phospherus/phospherus_directory.h>

#include<devices/block/blockDevice.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<returnValue.h>
#include<string.h>

typedef struct {
    Index64 inodeBlockIndex;
    iNodeType type;
    char reserved[52];
    char name[PHOSPHERUS_MAX_NAME_LENGTH + 1];
} __attribute__((packed)) __DirectoryEntry;

static ReturnValue __addEntry(Directory* this, iNode* entryInode, ConstCstring name);

static ReturnValue __removeEntry(Directory* this, Index64 entryIndex);

static Index64 __lookupEntry(Directory* this, ConstCstring name, iNodeType type);

static ReturnValue __readEntry(Directory* this, DirectoryEntry* entry, Index64 entryIndex);

DirectoryOperations directoryOperations = {
    .addEntry = __addEntry,
    .removeEntry = __removeEntry,
    .lookupEntry = __lookupEntry,
    .readEntry = __readEntry
};

static Directory* __openDirectory(iNode* iNode);

static ReturnValue __closeDirectory(Directory* directory);

DirectoryGlobalOperations directoryGlobalOperations = {
    .openDirectory = __openDirectory,
    .closeDirectory = __closeDirectory
};

DirectoryGlobalOperations* phospherusInitDirectories() {
    return &directoryGlobalOperations;
}

static ReturnValue __addEntry(Directory* this, iNode* entryInode, ConstCstring name) {
    if (__lookupEntry(this, name, entryInode->onDevice.type) != INVALID_INDEX) {
        return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_ITEM, RETURN_VALUE_STATUS_ALREADY_EXIST);
    }

    iNode* directoryInode = this->iNode;

    size_t 
        oldBlockSize = directoryInode->onDevice.availableBlockSize,
        newBlockSize = ((this->size + 1) * sizeof(__DirectoryEntry) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    void* newDirectoryInMemory = NULL;
    if (this->size == 0) {
        newDirectoryInMemory = kMalloc(sizeof(__DirectoryEntry), MEMORY_TYPE_NORMAL);
    } else if (oldBlockSize != newBlockSize) {
        newDirectoryInMemory = kRealloc(this->directoryInMemory, newBlockSize * BLOCK_SIZE);
    }

    if (newDirectoryInMemory == NULL) {
        return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_MEMORY, RETURN_VALUE_STATUS_NO_FREE_SPACE);
    }

    this->directoryInMemory = newDirectoryInMemory;

    __DirectoryEntry* newEntry = (__DirectoryEntry*)(this->directoryInMemory + this->size * sizeof(__DirectoryEntry));
    memset(newEntry, 0, sizeof(__DirectoryEntry));
    ++this->size;

    newEntry->inodeBlockIndex = entryInode->blockIndex;
    newEntry->type = entryInode->onDevice.type;
    strcpy(newEntry->name, name);

    ReturnValue res = RETURN_VALUE_RETURN_NORMALLY;
    if (newBlockSize != oldBlockSize && RETURN_VALUE_IS_ERROR(res = iNodeResize(directoryInode, newBlockSize))) {
        return res;
    }

    if (RETURN_VALUE_IS_ERROR(res = iNodeWriteBlocks(directoryInode, this->directoryInMemory + (newBlockSize - 1) * BLOCK_SIZE, newBlockSize - 1, 1))) {
        return res;
    }

    directoryInode->onDevice.dataSize = this->size * sizeof(__DirectoryEntry);
    blockDeviceWriteBlocks(directoryInode->device, directoryInode->blockIndex, &directoryInode->onDevice, 1);

    return RETURN_VALUE_RETURN_NORMALLY;
}

static ReturnValue __removeEntry(Directory* this, Index64 entryIndex) {
    iNode* directoryInode = this->iNode;

    size_t 
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
    }
    --this->size;

    ReturnValue res = RETURN_VALUE_RETURN_NORMALLY;
    if (newBlockSize != oldBlockSize && RETURN_VALUE_IS_ERROR(res = iNodeResize(directoryInode, newBlockSize))) {
        return res;
    }

    if (newBlockSize > 0 && RETURN_VALUE_IS_ERROR(res = iNodeWriteBlocks(directoryInode, this->directoryInMemory, 0, newBlockSize))) {
        return res;
    }

    directoryInode->onDevice.dataSize = this->size * sizeof(__DirectoryEntry);
    blockDeviceWriteBlocks(directoryInode->device, directoryInode->blockIndex, &directoryInode->onDevice, 1);

    return RETURN_VALUE_RETURN_NORMALLY;
}

static Index64 __lookupEntry(Directory* this, ConstCstring name, iNodeType type) {
    __DirectoryEntry* entries = this->directoryInMemory;
    for (int i = 0; i < this->size; ++i) {
        if (entries[i].type == type && strcmp(name, entries[i].name) == 0) {
            return i;
        }
    }

    return INVALID_INDEX;
}

static ReturnValue __readEntry(Directory* this, DirectoryEntry* entry, Index64 entryIndex) {
    if (entryIndex >= this->size) {
        return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_INDEX, RETURN_VALUE_STATUS_OUT_OF_BOUND);
    }

    __DirectoryEntry* diectoryEntry = ((__DirectoryEntry*)this->directoryInMemory) + entryIndex;
    entry->name = diectoryEntry->name;
    entry->iNodeIndex = diectoryEntry->inodeBlockIndex;
    entry->type = diectoryEntry->type;

    return RETURN_VALUE_RETURN_NORMALLY;
}

static Directory* __openDirectory(iNode* iNode) {
    if (iNode->onDevice.type != INODE_TYPE_DIRECTORY) {
        return NULL;
    }

    if (iNode->entryReference != NULL) {
        ++iNode->referenceCnt;
        return iNode->entryReference;
    }

    Directory* ret = kMalloc(sizeof(Directory), MEMORY_TYPE_NORMAL);
    ret->iNode = iNode;
    ret->size = iNode->onDevice.dataSize / sizeof(__DirectoryEntry);
    ret->operations = &directoryOperations;
    ret->directoryInMemory = NULL;
    if (ret->size > 0) {
        ret->directoryInMemory = kMalloc(iNode->onDevice.availableBlockSize * BLOCK_SIZE, MEMORY_TYPE_NORMAL);
        iNodeReadBlocks(iNode, ret->directoryInMemory, 0, iNode->onDevice.availableBlockSize);
    }

    return ret;
}

static ReturnValue __closeDirectory(Directory* directory) {
    iNode* iNode = directory->iNode;
    if (iNode->entryReference == NULL) {
        return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_DATA, RETURN_VALUE_STATUS_NOT_FOUND);
    }

    if (--iNode->referenceCnt == 0) {
        iNode->entryReference = NULL;
        directory->iNode = NULL;
        kFree(directory->directoryInMemory);
        kFree(directory);
    }

    return RETURN_VALUE_RETURN_NORMALLY;
}
