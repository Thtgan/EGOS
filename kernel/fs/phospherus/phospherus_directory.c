#include<fs/phospherus/phospherus_directory.h>

#include<devices/block/blockDevice.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/virtualMalloc.h>
#include<string.h>

typedef struct {
    Index64 inodeBlockIndex;
    iNodeType type;
    char reserved[52];
    char name[PHOSPHERUS_MAX_NAME_LENGTH + 1];
} __attribute__((packed)) __DirectoryEntry;

int __addEntry(Directory* directory, iNode* entryInode, ConstCstring name);

int __removeEntry(Directory* directory, Index64 entryIndex);

Index64 __lookupEntry(Directory* directory, ConstCstring name, iNodeType type);

DirectoryEntry* __getEntry(Directory* directory, Index64 entryIndex);

DirectoryOperations directoryOperations = {
    .addEntry = __addEntry,
    .removeEntry = __removeEntry,
    .lookupEntry = __lookupEntry,
    .getEntry = __getEntry
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

int __addEntry(Directory* directory, iNode* entryInode, ConstCstring name) {
    if (__lookupEntry(directory, name, entryInode->onDevice.type) != PHOSPHERUS_NULL) {
        return -1;
    }

    iNode* directoryInode = directory->iNode;

    size_t 
        oldBlockSize = directoryInode->onDevice.availableBlockSize,
        newBlockSize = ((directory->size + 1) * sizeof(__DirectoryEntry) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (directory->size == 0) {
        directory->directoryInMemory = vMalloc(sizeof(__DirectoryEntry));
    } else if (oldBlockSize != newBlockSize) {
        directory->directoryInMemory = vRealloc(directory->directoryInMemory, newBlockSize * BLOCK_SIZE);
    }

    __DirectoryEntry* newEntry = (__DirectoryEntry*)(directory->directoryInMemory + directory->size * sizeof(__DirectoryEntry));
    memset(newEntry, 0, sizeof(__DirectoryEntry));
    ++directory->size;

    newEntry->inodeBlockIndex = entryInode->blockIndex;
    newEntry->type = entryInode->onDevice.type;
    strcpy(newEntry->name, name);

    if (newBlockSize != oldBlockSize) {
        if (directoryInode->operations->resize(directoryInode, newBlockSize) == -1) {
            return -1;
        }
    }

    if (directoryInode->operations->writeBlocks(directoryInode, directory->directoryInMemory + (newBlockSize - 1) * BLOCK_SIZE, newBlockSize - 1, 1) == -1) {
        return -1;
    }

    directoryInode->onDevice.dataSize = directory->size * sizeof(__DirectoryEntry);
    THIS_ARG_APPEND_CALL(directoryInode->device, operations->writeBlocks, directoryInode->blockIndex, &directoryInode->onDevice, 1);

    return 0;
}

int __removeEntry(Directory* directory, Index64 entryIndex) {
    iNode* directoryInode = directory->iNode;

    size_t 
        oldBlockSize = directoryInode->onDevice.availableBlockSize,
        newBlockSize = ((directory->size - 1) * sizeof(__DirectoryEntry) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (directory->size == 1) {
        vFree(directory->directoryInMemory);
        directory->directoryInMemory = NULL;
    } else {
        memmove(
            directory->directoryInMemory + entryIndex * sizeof(__DirectoryEntry),
            directory->directoryInMemory + (entryIndex + 1) * sizeof(__DirectoryEntry),
            (directory->size - entryIndex - 1) * sizeof(__DirectoryEntry)
            );

        directory->directoryInMemory = vRealloc(directory->directoryInMemory, (directory->size - 1) * sizeof(__DirectoryEntry));
    }
    --directory->size;

    if (newBlockSize != oldBlockSize) {
        if (directoryInode->operations->resize(directoryInode, newBlockSize) == -1) {
            return -1;
        }
    }

    if (newBlockSize > 0) {
        if (directoryInode->operations->writeBlocks(directoryInode, directory->directoryInMemory, 0, newBlockSize) == -1) {
            return -1;
        }
    }

    directoryInode->onDevice.dataSize = directory->size * sizeof(__DirectoryEntry);
    THIS_ARG_APPEND_CALL(directoryInode->device, operations->writeBlocks, directoryInode->blockIndex, &directoryInode->onDevice, 1);

    return 0;
}

Index64 __lookupEntry(Directory* directory, ConstCstring name, iNodeType type) {
    __DirectoryEntry* entries = directory->directoryInMemory;
    for (int i = 0; i < directory->size; ++i) {
        if (entries[i].type == type && strcmp(name, entries[i].name) == 0) {
            return i;
        }
    }

    return PHOSPHERUS_NULL;
}

DirectoryEntry* __getEntry(Directory* directory, Index64 entryIndex) {
    if (entryIndex >= directory->size) {
        return NULL;
    }

    DirectoryEntry* ret = vMalloc(sizeof(DirectoryEntry));
    __DirectoryEntry* entry = directory->directoryInMemory + entryIndex;
    ret->name = entry->name;
    ret->iNodeIndex = entry->inodeBlockIndex;
    ret->type = entry->type;

    return ret;
}

Directory* __openDirectory(iNode* iNode) {
    if (iNode->onDevice.type != INODE_TYPE_DIRECTORY) {
        return NULL;
    }

    if (iNode->entryReference != NULL) {
        ++iNode->referenceCnt;
        return iNode->entryReference;
    }

    Directory* ret = vMalloc(sizeof(Directory));
    ret->iNode = iNode;
    ret->size = iNode->onDevice.dataSize / sizeof(__DirectoryEntry);
    ret->operations = &directoryOperations;
    ret->directoryInMemory = NULL;
    if (ret->size > 0) {
        ret->directoryInMemory = vMalloc(iNode->onDevice.availableBlockSize * BLOCK_SIZE);
        iNode->operations->readBlocks(iNode, ret->directoryInMemory, 0, iNode->onDevice.availableBlockSize);
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
        vFree(directory->directoryInMemory);
        vFree(directory);
    }

    return 0;
}
