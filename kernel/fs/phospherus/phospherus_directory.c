#include<fs/phospherus/phospherus_directory.h>

#include<devices/block/blockDevice.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<string.h>

typedef struct {
    Index64 inodeBlockIndex;
    iNodeType type;
    char reserved[52];
    char name[PHOSPHERUS_MAX_NAME_LENGTH + 1];
} __attribute__((packed)) __DirectoryEntry;

int __addEntry(Directory* this, iNode* entryInode, ConstCstring name);

int __removeEntry(Directory* this, Index64 entryIndex);

Index64 __lookupEntry(Directory* this, ConstCstring name, iNodeType type);

int __readEntry(Directory* this, DirectoryEntry* entry, Index64 entryIndex);

DirectoryOperations directoryOperations = {
    .addEntry = __addEntry,
    .removeEntry = __removeEntry,
    .lookupEntry = __lookupEntry,
    .readEntry = __readEntry
};

Directory* __openDirectory(iNode* iNode);

int __closeDirectory(Directory* directory);

DirectoryGlobalOperations directoryGlobalOperations = {
    .openDirectory = __openDirectory,
    .closeDirectory = __closeDirectory
};

DirectoryGlobalOperations* phospherusInitDirectories() {
    return &directoryGlobalOperations;
}

int __addEntry(Directory* this, iNode* entryInode, ConstCstring name) {
    if (__lookupEntry(this, name, entryInode->onDevice.type) != PHOSPHERUS_NULL) {
        return -1;
    }

    iNode* directoryInode = this->iNode;

    size_t 
        oldBlockSize = directoryInode->onDevice.availableBlockSize,
        newBlockSize = ((this->size + 1) * sizeof(__DirectoryEntry) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (this->size == 0) {
        this->directoryInMemory = kMalloc(sizeof(__DirectoryEntry), MEMORY_TYPE_NORMAL);
    } else if (oldBlockSize != newBlockSize) {
        this->directoryInMemory = kRealloc(this->directoryInMemory, newBlockSize * BLOCK_SIZE);
    }

    __DirectoryEntry* newEntry = (__DirectoryEntry*)(this->directoryInMemory + this->size * sizeof(__DirectoryEntry));
    memset(newEntry, 0, sizeof(__DirectoryEntry));
    ++this->size;

    newEntry->inodeBlockIndex = entryInode->blockIndex;
    newEntry->type = entryInode->onDevice.type;
    strcpy(newEntry->name, name);

    if (newBlockSize != oldBlockSize) {
        if (iNodeResize(directoryInode, newBlockSize) == -1) {
            return -1;
        }
    }

    if (iNodeWriteBlocks(directoryInode, this->directoryInMemory + (newBlockSize - 1) * BLOCK_SIZE, newBlockSize - 1, 1) == -1) {
        return -1;
    }

    directoryInode->onDevice.dataSize = this->size * sizeof(__DirectoryEntry);
    blockDeviceWriteBlocks(directoryInode->device, directoryInode->blockIndex, &directoryInode->onDevice, 1);

    return 0;
}

int __removeEntry(Directory* this, Index64 entryIndex) {
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

    if (newBlockSize != oldBlockSize) {
        if (iNodeResize(directoryInode, newBlockSize) == -1) {
            return -1;
        }
    }

    if (newBlockSize > 0) {
        if (iNodeWriteBlocks(directoryInode, this->directoryInMemory, 0, newBlockSize) == -1) {
            return -1;
        }
    }

    directoryInode->onDevice.dataSize = this->size * sizeof(__DirectoryEntry);
    blockDeviceWriteBlocks(directoryInode->device, directoryInode->blockIndex, &directoryInode->onDevice, 1);

    return 0;
}

Index64 __lookupEntry(Directory* this, ConstCstring name, iNodeType type) {
    __DirectoryEntry* entries = this->directoryInMemory;
    for (int i = 0; i < this->size; ++i) {
        if (entries[i].type == type && strcmp(name, entries[i].name) == 0) {
            return i;
        }
    }

    return PHOSPHERUS_NULL;
}

int __readEntry(Directory* this, DirectoryEntry* entry, Index64 entryIndex) {
    if (entryIndex >= this->size) {
        return -1;
    }

    __DirectoryEntry* diectoryEntry = ((__DirectoryEntry*)this->directoryInMemory) + entryIndex;
    entry->name = diectoryEntry->name;
    entry->iNodeIndex = diectoryEntry->inodeBlockIndex;
    entry->type = diectoryEntry->type;

    return 0;
}

Directory* __openDirectory(iNode* iNode) {
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

int __closeDirectory(Directory* directory) {
    iNode* iNode = directory->iNode;
    if (iNode->entryReference == NULL) {
        return -1;
    }

    if (--iNode->referenceCnt == 0) {
        iNode->entryReference = NULL;
        directory->iNode = NULL;
        kFree(directory->directoryInMemory);
        kFree(directory);
    }

    return 0;
}
