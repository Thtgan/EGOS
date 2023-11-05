#include<fs/fileSystemEntry.h>

#include<algorithms.h>
#include<devices/block/blockDevice.h>
#include<fs/fsutil.h>
#include<fs/fileSystem.h>
#include<fs/fsPreDefines.h>
#include<fs/inode.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<string.h>
#include<system/pageTable.h>

Result initFileSystemEntryDescriptor(FileSystemEntryDescriptor* desc, FileSystemEntryDescriptorInitArgs* args) {
    ConstCstring name = args->name;

    FileSystemEntryIdentifier* identifier = &desc->identifier;
    if (name == NULL) {
        identifier->name    = NULL;
    } else {
        Cstring copy = kMalloc(strlen(name) + 1);
        if (copy == NULL) {
            return RESULT_FAIL;
        }

        strcpy(copy, name);
        identifier->name    = copy;
    }

    identifier->type        = args->type;
    identifier->parent      = args->parent;
    desc->dataRange         = args->dataRange;
    desc->flags             = args->flags;
    desc->entry             = NULL;

    return RESULT_SUCCESS;
}

void clearFileSystemEntryDescriptor(FileSystemEntryDescriptor* desc) {
    if (desc->identifier.name != NULL) {
        kFree((void*)desc->identifier.name);
    }

    memset(desc, 0, sizeof(FileSystemEntryDescriptor));
}

Result genericOpenFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry, FileSystemEntryDescriptor* entryDescripotor) {
    //TODO: What if descriptor already open?

    entry->descriptor       = entryDescripotor;
    
    entry->pointer          = 0;
    iNode* iNodePtr         = kMalloc(sizeof(iNode));
    if (rawSuperNodeOpenInode(superBlock, iNodePtr, entryDescripotor) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    entry->iNode            = iNodePtr;

    entryDescripotor->entry = entry;
    SET_FLAG_BACK(entryDescripotor->flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_PRESENT);

    return RESULT_SUCCESS;
}

Result genericCloseFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry) {
    FileSystemEntryDescriptor* entryDescripotor = entry->descriptor;

    CLEAR_FLAG_BACK(entryDescripotor->flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_PRESENT);
    entryDescripotor->entry = NULL;

    if (rawSuperNodeCloseInode(superBlock, entry->iNode) == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    kFree(entry->iNode);

    memset(entry, 0, sizeof(FileSystemEntry));

    return RESULT_SUCCESS;
}

Index64 genericFileSystemEntrySeek(FileSystemEntry* entry, Index64 seekTo) {
    if (seekTo > entry->descriptor->dataRange.length) {
        return INVALID_INDEX;
    }
    
    return entry->pointer = seekTo;
}

Result genericFileSystemEntryRead(FileSystemEntry* entry, void* buffer, Size n) {
    iNode* iNode = entry->iNode;
    SuperBlock* superBlock = iNode->superBlock;
    BlockDevice* device = superBlock->device;

    void* blockBuffer = allocateBuffer(device->bytePerBlockShift);
    if (blockBuffer == NULL) {
        return RESULT_FAIL;
    }

    Index64 blockIndex = DIVIDE_ROUND_DOWN_SHIFT(entry->pointer, device->bytePerBlockShift), offsetInBlock = entry->pointer % POWER_2(device->bytePerBlockShift);
    Size blockSize = POWER_2(device->bytePerBlockShift), remainByteNum = min64(n, entry->descriptor->dataRange.length - entry->pointer);
    Range range;

    if (offsetInBlock != 0) {
        Size tmpN = 1;
        if (rawInodeMapBlockPosition(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (blockDeviceReadBlocks(device, range.begin, blockBuffer, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        Size byteReadN = min64(remainByteNum, POWER_2(device->bytePerBlockShift) - offsetInBlock);
        memcpy(buffer, blockBuffer + offsetInBlock, byteReadN);

        buffer += byteReadN;
        remainByteNum -= byteReadN;
    }

    if (remainByteNum >= blockSize) {
        Size remainBlockNum = DIVIDE_ROUND_DOWN_SHIFT(remainByteNum, device->bytePerBlockShift);
        Result res;
        while ((res = rawInodeMapBlockPosition(iNode, &blockIndex, &remainBlockNum, &range, 1)) == RESULT_CONTINUE) {
            if (blockDeviceReadBlocks(device, range.begin, buffer, range.length) == RESULT_FAIL) {
                return RESULT_FAIL;
            }

            Size byteReadN = range.length << device->bytePerBlockShift;
            buffer += byteReadN;
            remainByteNum -= byteReadN;
        }

        if (res == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    if (remainByteNum > 0) {
        Size tmpN = 1;
        if (rawInodeMapBlockPosition(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (blockDeviceReadBlocks(device, range.begin, blockBuffer, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        memcpy(buffer, blockBuffer, remainByteNum);

        remainByteNum = 0;
    }

    releaseBuffer(blockBuffer, device->bytePerBlockShift);

    return RESULT_SUCCESS;
}

Result genericFileSystemEntryWrite(FileSystemEntry* entry, const void* buffer, Size n) {
    iNode* iNode = entry->iNode;
    SuperBlock* superBlock = iNode->superBlock;
    BlockDevice* device = superBlock->device;

    void* blockBuffer = allocateBuffer(device->bytePerBlockShift);
    if (blockBuffer == NULL) {
        return RESULT_FAIL;
    }

    Index64 blockIndex = DIVIDE_ROUND_DOWN_SHIFT(entry->pointer, device->bytePerBlockShift), offsetInBlock = entry->pointer % POWER_2(device->bytePerBlockShift);
    Size blockSize = POWER_2(device->bytePerBlockShift), remainByteNum = n;
    if (entry->pointer + n > entry->descriptor->dataRange.length) {
        if (rawFileSystemEntryResize(entry, entry->pointer + n) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    Range range;
    if (offsetInBlock != 0) {
        Size tmpN = 1;
        if (rawInodeMapBlockPosition(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (blockDeviceReadBlocks(device, range.begin, blockBuffer, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        Size byteWriteN = min64(remainByteNum, POWER_2(device->bytePerBlockShift) - offsetInBlock);
        memcpy(blockBuffer + offsetInBlock, buffer, byteWriteN);

        if (blockDeviceWriteBlocks(device, range.begin, blockBuffer, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        buffer += byteWriteN;
        remainByteNum -= byteWriteN;
    }

    if (remainByteNum >= blockSize) {
        Size remainBlockNum = DIVIDE_ROUND_DOWN_SHIFT(remainByteNum, device->bytePerBlockShift);
        Result res;
        while ((res = rawInodeMapBlockPosition(iNode, &blockIndex, &remainBlockNum, &range, 1)) == RESULT_CONTINUE) {
            if (blockDeviceWriteBlocks(device, range.begin, buffer, range.length) == RESULT_FAIL) {
                return RESULT_FAIL;
            }

            Size byteWriteN = range.length << device->bytePerBlockShift;
            buffer += byteWriteN;
            remainByteNum -= byteWriteN;
        }

        if (res == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    if (remainByteNum > 0) {
        Size tmpN = 1;
        if (rawInodeMapBlockPosition(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (blockDeviceReadBlocks(device, range.begin, blockBuffer, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        memcpy(blockBuffer, buffer, remainByteNum);

        if (blockDeviceWriteBlocks(device, range.begin, blockBuffer, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        remainByteNum = 0;
    }

    releaseBuffer(blockBuffer, device->bytePerBlockShift);

    return RESULT_SUCCESS;
}

Result fileSystemEntryOpen(FileSystemEntry* entry, FileSystemEntryDescriptor* desc, ConstCstring path, FileSystemEntryType type) {
    char* buffer = allocateBuffer(BUFFER_SIZE_512);

    Result res = RESULT_SUCCESS;

    SuperBlock* superBlock = rootFileSystem->superBlock;
    Directory currentDirectory = superBlock->rootDirectory;
    FileSystemEntry tmpEntry;
    while (true) {
        if (*path != '\\') {    //TODO: Seperator should be '/'
            res = RESULT_FAIL;
            break;
        }
        ++path;

        ConstCstring next = path;
        while (*next != '\\' && *next != '\0') {
            ++next;
        }

        Size len = ARRAY_POINTER_TO_INDEX(path, next);
        if (len >= POWER_2(BUFFER_SIZE_512)) {
            res = RESULT_FAIL;
            break;
        }

        strncpy(buffer, path, len);
        
        FileSystemEntryIdentifier identifier = {
            .name   = buffer,
            .type   = *next == '\0' ? type : FILE_SYSTEM_ENTRY_TYPE_DIRECTOY,
            .parent = entry->descriptor
        };

        if (directoryLookup(currentDirectory, desc, NULL, &identifier) != RESULT_SUCCESS) {
            res = RESULT_FAIL;
            break;
        }

        if (rawSuperNodeOpenFileSystemEntry(superBlock, &tmpEntry, desc) == RESULT_FAIL) {
            res = RESULT_FAIL;
            break;
        }
        
        if (*next == '\0') {
            memcpy(entry, &tmpEntry, sizeof(FileSystemEntry));
            break;
        } else {
            path = next;
            if (currentDirectory != superBlock->rootDirectory) {
                if (superBlock->operations->closeFileSystemEntry(superBlock, entry) == RESULT_FAIL) {
                    res = RESULT_FAIL;
                    break;
                }
            }
            currentDirectory = &tmpEntry;
        }
    }

    releaseBuffer(buffer, BUFFER_SIZE_512);

    return res;
}

Result fileSystemEntryClose(FileSystemEntry* entry) {
    SuperBlock* superBlock = entry->iNode->superBlock;
    FileSystemEntryIdentifier* identifier = &entry->descriptor->identifier;
    if (identifier->type != FILE_SYSTEM_ENTRY_TYPE_DIRECTOY) {
        FileSystemEntry* parentEntry = identifier->parent->entry;
        if (parentEntry == NULL || rawFileSystemEntryUpdateChild(parentEntry, identifier, entry->descriptor) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return superBlock->operations->closeFileSystemEntry(superBlock, entry);
}