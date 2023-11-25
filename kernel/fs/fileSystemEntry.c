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

    desc->createTime        = args->createTime;
    desc->lastAccessTime    = args->lastAccessTime;
    desc->lastModifyTime    = args->lastModifyTime;

    return RESULT_SUCCESS;
}

void clearFileSystemEntryDescriptor(FileSystemEntryDescriptor* desc) {
    if (desc->identifier.name != NULL) {
        kFree((void*)desc->identifier.name);
    }

    memset(desc, 0, sizeof(FileSystemEntryDescriptor));
}

Result genericOpenFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry, FileSystemEntryDescriptor* entryDescriptor) {
    entry->descriptor       = entryDescriptor;
    entry->pointer          = 0;
    entry->iNode            = openInodeBuffered(superBlock, entryDescriptor);
    if (entry->iNode == NULL) {
        return RESULT_FAIL;
    }

    SET_FLAG_BACK(entryDescriptor->flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_PRESENT);

    return RESULT_SUCCESS;
}

Result genericCloseFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry) {
    FileSystemEntryDescriptor* entryDescriptor = entry->descriptor;

    CLEAR_FLAG_BACK(entryDescriptor->flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_PRESENT);

    if (closeInodeBuffered(entry->iNode, entryDescriptor) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

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