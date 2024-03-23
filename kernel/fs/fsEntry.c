#include<fs/fsEntry.h>

#include<algorithms.h>
#include<cstring.h>
#include<devices/block/blockDevice.h>
#include<fs/fsutil.h>
#include<fs/fs.h>
#include<fs/fsStructs.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<structs/string.h>
#include<system/pageTable.h>

Result fsEntryIdentifier_initStruct(fsEntryIdentifier* identifier, ConstCstring path, fsEntryType type) {
    ConstCstring sep = strrchr(path, FS_PATH_SEPERATOR);
    if (sep == NULL) {
        return RESULT_FAIL;
    }
    if (
        string_initStructN(&identifier->parentPath, path, ARRAY_POINTER_TO_INDEX(path, sep)) == RESULT_FAIL ||
        string_initStruct(&identifier->name, sep + 1) == RESULT_FAIL
    ) {
        return RESULT_FAIL; //TODO: Memory release if failed here
    }

    identifier->type = type;

    return RESULT_SUCCESS;
}

Result fsEntryIdentifier_initStructSep(fsEntryIdentifier* identifier, ConstCstring parentPath, ConstCstring name, fsEntryType type) {
    if (
        string_initStruct(&identifier->parentPath, parentPath) == RESULT_FAIL ||
        string_initStruct(&identifier->name, name) == RESULT_FAIL
    ) {
        return RESULT_FAIL; //TODO: Memory release if failed here
    }

    identifier->type = type;

    return RESULT_SUCCESS;
}

void fsEntryIdentifier_clearStruct(fsEntryIdentifier* identifier) {
    string_clearStruct(&identifier->parentPath);
    string_clearStruct(&identifier->name);
}

Result fsEntryIdentifier_getParent(fsEntryIdentifier* identifier, fsEntryIdentifier* parentIdentifierOut) {
    return fsEntryIdentifier_initStruct(parentIdentifierOut, identifier->parentPath.data, FS_ENTRY_TYPE_DIRECTORY);
}

Result fsEntryDesc_initStruct(fsEntryDesc* desc, fsEntryDescInitArgs* args) {
    if (args->name == NULL || args->parentPath == NULL) {
        return RESULT_FAIL;
    }

    fsEntryIdentifier* identifier = &desc->identifier;
    if (fsEntryIdentifier_initStructSep(identifier, args->parentPath, args->name, args->type) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (args->isDevice) {
        desc->device        = args->device;
    } else {
        desc->dataRange     = args->dataRange;
    }
    desc->flags             = args->flags;
    desc->createTime        = args->createTime;
    desc->lastAccessTime    = args->lastAccessTime;
    desc->lastModifyTime    = args->lastModifyTime;

    hashChainNode_initStruct(&desc->descNode);
    desc->descReferCnt      = 0;

    return RESULT_SUCCESS;
}

void fsEntryDesc_clearStruct(fsEntryDesc* desc) {
    fsEntryIdentifier_clearStruct(&desc->identifier);
    memset(desc, 0, sizeof(fsEntryDesc));
}

Result fsEntry_genericOpen(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc) {
    entry->desc     = desc;
    entry->pointer  = 0;
    entry->iNode    = iNode_open(superBlock, desc);
    if (entry->iNode == NULL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result fsEntry_genericClose(SuperBlock* superBlock, fsEntry* entry) {
    fsEntryDesc* desc = entry->desc;

    if (iNode_close(entry->iNode, desc) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    memset(entry, 0, sizeof(fsEntry));

    return RESULT_SUCCESS;
}

Index64 fsEntry_genericSeek(fsEntry* entry, Index64 seekTo) {
    if (seekTo > entry->desc->dataRange.length) {
        return INVALID_INDEX;
    }
    
    return entry->pointer = seekTo;
}

Result fsEntry_genericRead(fsEntry* entry, void* buffer, Size n) {
    iNode* iNode = entry->iNode;
    SuperBlock* superBlock = iNode->superBlock;
    BlockDevice* device = superBlock->device;

    void* blockBuffer = allocateBuffer(device->bytePerBlockShift);
    if (blockBuffer == NULL) {
        return RESULT_FAIL;
    }

    Index64 blockIndex = DIVIDE_ROUND_DOWN_SHIFT(entry->pointer, device->bytePerBlockShift), offsetInBlock = entry->pointer % POWER_2(device->bytePerBlockShift);
    Size blockSize = POWER_2(device->bytePerBlockShift), remainByteNum = min64(n, entry->desc->dataRange.length - entry->pointer);
    Range range;

    if (offsetInBlock != 0) {
        Size tmpN = 1;
        if (iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_FAIL) {
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
        while ((res = iNode_rawTranslateBlockPos(iNode, &blockIndex, &remainBlockNum, &range, 1)) == RESULT_CONTINUE) {
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
        if (iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_FAIL) {
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

Result fsEntry_genericWrite(fsEntry* entry, const void* buffer, Size n) {
    iNode* iNode = entry->iNode;
    SuperBlock* superBlock = iNode->superBlock;
    BlockDevice* device = superBlock->device;

    void* blockBuffer = allocateBuffer(device->bytePerBlockShift);
    if (blockBuffer == NULL) {
        return RESULT_FAIL;
    }

    Index64 blockIndex = DIVIDE_ROUND_DOWN_SHIFT(entry->pointer, device->bytePerBlockShift), offsetInBlock = entry->pointer % POWER_2(device->bytePerBlockShift);
    Size blockSize = POWER_2(device->bytePerBlockShift), remainByteNum = n;
    if (entry->pointer + n > entry->desc->dataRange.length) {
        if (fsEntry_rawResize(entry, entry->pointer + n) == RESULT_FAIL) {  //TODO: May be remove this?
            return RESULT_FAIL;
        }
    }

    Range range;
    if (offsetInBlock != 0) {
        Size tmpN = 1;
        if (iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_FAIL) {
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
        while ((res = iNode_rawTranslateBlockPos(iNode, &blockIndex, &remainBlockNum, &range, 1)) == RESULT_CONTINUE) {
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
        if (iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_FAIL) {
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

Result fsEntry_genericResize(fsEntry* entry, Size newSizeInByte) {
    if (iNode_rawResize(entry->iNode, newSizeInByte) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    entry->desc->dataRange.length = newSizeInByte;

    return RESULT_SUCCESS;
}