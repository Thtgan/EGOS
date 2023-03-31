#include<fs/phospherus/phospherus_directory.h>

#include<devices/block/blockDevice.h>
#include<error.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<kit/types.h>
#include<malloc.h>
#include<memory.h>
#include<string.h>

typedef struct {
    ID iNodeID;
    iNodeType type;
    char reserved[52];
    char name[PHOSPHERUS_MAX_NAME_LENGTH + 1];
} __attribute__((packed)) __DirectoryEntry;

static int __addEntry(Directory* this, ID iNodeID, iNodeType type, ConstCstring name);

static int __removeEntry(Directory* this, Index64 entryIndex);

static Index64 __lookupEntry(Directory* this, ConstCstring name, iNodeType type);

static int __readEntry(Directory* this, DirectoryEntry* entry, Index64 entryIndex);

DirectoryOperations directoryOperations = {
    .addEntry = __addEntry,
    .removeEntry = __removeEntry,
    .lookupEntry = __lookupEntry,
    .readEntry = __readEntry
};

static Directory* __openDirectory(iNode* iNode);

static int __closeDirectory(Directory* directory);

DirectoryGlobalOperations directoryGlobalOperations = {
    .openDirectory = __openDirectory,
    .closeDirectory = __closeDirectory
};

DirectoryGlobalOperations* phospherusInitDirectories() {
    return &directoryGlobalOperations;
}

static int __addEntry(Directory* this, ID iNodeID, iNodeType type, ConstCstring name) {
    if (__lookupEntry(this, name, type) != INVALID_INDEX) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
        return -1;
    }

    iNode* directoryInode = this->iNode;

    size_t 
        oldBlockSize = directoryInode->onDevice.availableBlockSize,
        newBlockSize = ((this->size + 1) * sizeof(__DirectoryEntry) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    void* newDirectoryInMemory = NULL;
    if (this->size == 0) {
        newDirectoryInMemory = malloc(sizeof(__DirectoryEntry));
    } else if (oldBlockSize != newBlockSize) {
        newDirectoryInMemory = realloc(this->directoryInMemory, newBlockSize * BLOCK_SIZE);
    }

    if (newDirectoryInMemory == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_MEMORY, ERROR_STATUS_NO_FREE_SPACE);
        return -1;
    }

    this->directoryInMemory = newDirectoryInMemory;

    __DirectoryEntry* newEntry = (__DirectoryEntry*)(this->directoryInMemory + this->size * sizeof(__DirectoryEntry));
    memset(newEntry, 0, sizeof(__DirectoryEntry));
    ++this->size;

    newEntry->iNodeID = iNodeID;
    newEntry->type = type;
    strcpy(newEntry->name, name);

    if (newBlockSize != oldBlockSize && iNodeResize(directoryInode, newBlockSize) == -1) {
        return -1;
    }

    if (iNodeWriteBlocks(directoryInode, this->directoryInMemory + (newBlockSize - 1) * BLOCK_SIZE, newBlockSize - 1, 1) == -1) {
        return -1;
    }

    directoryInode->onDevice.dataSize = this->size * sizeof(__DirectoryEntry);
    blockDeviceWriteBlocks(directoryInode->device, directoryInode->blockIndex, &directoryInode->onDevice, 1);

    return 0;
}

static int __removeEntry(Directory* this, Index64 entryIndex) {
    iNode* directoryInode = this->iNode;

    size_t 
        oldBlockSize = directoryInode->onDevice.availableBlockSize,
        newBlockSize = ((this->size - 1) * sizeof(__DirectoryEntry) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (this->size == 1) {
        free(this->directoryInMemory);
        this->directoryInMemory = NULL;
    } else {
        memmove(
            this->directoryInMemory + entryIndex * sizeof(__DirectoryEntry),
            this->directoryInMemory + (entryIndex + 1) * sizeof(__DirectoryEntry),
            (this->size - entryIndex - 1) * sizeof(__DirectoryEntry)
            );

        this->directoryInMemory = realloc(this->directoryInMemory, (this->size - 1) * sizeof(__DirectoryEntry));
    }
    --this->size;

    if (newBlockSize != oldBlockSize && iNodeResize(directoryInode, newBlockSize) == -1) {
        return -1;
    }

    if (newBlockSize > 0 && iNodeWriteBlocks(directoryInode, this->directoryInMemory, 0, newBlockSize) == -1) {
        return -1;
    }

    directoryInode->onDevice.dataSize = this->size * sizeof(__DirectoryEntry);
    blockDeviceWriteBlocks(directoryInode->device, directoryInode->blockIndex, &directoryInode->onDevice, 1);

    return 0;
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

static int __readEntry(Directory* this, DirectoryEntry* entry, Index64 entryIndex) {
    if (entryIndex >= this->size) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return -1;
    }

    __DirectoryEntry* diectoryEntry = ((__DirectoryEntry*)this->directoryInMemory) + entryIndex;
    entry->name = diectoryEntry->name;
    entry->iNodeID = diectoryEntry->iNodeID;
    entry->type = diectoryEntry->type;

    return 0;
}

static Directory* __openDirectory(iNode* iNode) {
    if (iNode->onDevice.type != INODE_TYPE_DIRECTORY) {
        return NULL;
    }

    if (iNode->entryReference != NULL) {
        ++iNode->referenceCnt;
        return iNode->entryReference;
    }

    Directory* ret = malloc(sizeof(Directory));
    ret->iNode = iNode;
    ret->size = iNode->onDevice.dataSize / sizeof(__DirectoryEntry);
    ret->operations = &directoryOperations;
    ret->directoryInMemory = NULL;
    if (ret->size > 0) {
        ret->directoryInMemory = malloc(iNode->onDevice.availableBlockSize * BLOCK_SIZE);
        iNodeReadBlocks(iNode, ret->directoryInMemory, 0, iNode->onDevice.availableBlockSize);
    }

    return ret;
}

static int __closeDirectory(Directory* directory) {
    iNode* iNode = directory->iNode;
    if (iNode->entryReference == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DATA, ERROR_STATUS_NOT_FOUND);
        return -1;
    }

    if (--iNode->referenceCnt == 0) {
        iNode->entryReference = NULL;
        directory->iNode = NULL;
        free(directory->directoryInMemory);
        free(directory);
    }

    return 0;
}
