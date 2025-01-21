#include<fs/fsEntry.h>

#include<algorithms.h>
#include<cstring.h>
#include<devices/block/blockDevice.h>
#include<devices/char/charDevice.h>
#include<fs/fsutil.h>
#include<fs/fs.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/string.h>
#include<system/pageTable.h>
#include<error.h>

OldResult fsEntryIdentifier_initStruct(fsEntryIdentifier* identifier, ConstCstring path, bool isDirectory) {
    if (*path == '\0') {
        if (
            string_initStruct(&identifier->parentPath, "") != RESULT_SUCCESS ||
            string_initStruct(&identifier->name, "") != RESULT_SUCCESS
        ) {
            return RESULT_ERROR; //TODO: Memory release if failed here
        }
    } else {
        ConstCstring sep = cstring_strrchr(path, FS_PATH_SEPERATOR);
        if (sep == NULL) {
            return RESULT_ERROR;
        }
        if (
            string_initStructN(&identifier->parentPath, path, ARRAY_POINTER_TO_INDEX(path, sep)) != RESULT_SUCCESS ||
            string_initStruct(&identifier->name, sep + 1) != RESULT_SUCCESS
        ) {
            return RESULT_ERROR; //TODO: Memory release if failed here
        }
    }

    identifier->isDirectory = isDirectory;

    return RESULT_SUCCESS;
}

OldResult fsEntryIdentifier_initStructSep(fsEntryIdentifier* identifier, ConstCstring parentPath, ConstCstring name, bool isDirectory) {
    if (
        string_initStruct(&identifier->parentPath, parentPath) != RESULT_SUCCESS ||
        string_initStruct(&identifier->name, name) != RESULT_SUCCESS
    ) {
        return RESULT_ERROR; //TODO: Memory release if failed here
    }

    identifier->isDirectory = isDirectory;

    return RESULT_SUCCESS;
}

void fsEntryIdentifier_clearStruct(fsEntryIdentifier* identifier) {
    string_clearStruct(&identifier->parentPath);
    string_clearStruct(&identifier->name);
    identifier->isDirectory = false;
}

OldResult fsEntryIdentifier_copy(fsEntryIdentifier* des, fsEntryIdentifier* src) {
    if (string_copy(&des->name, &src->name) != RESULT_SUCCESS || string_copy(&des->parentPath, &src->parentPath) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }
    
    des->isDirectory = src->isDirectory;

    return RESULT_SUCCESS;
}

OldResult fsEntryIdentifier_getParent(fsEntryIdentifier* identifier, fsEntryIdentifier* parentIdentifierOut) {
    return fsEntryIdentifier_initStruct(parentIdentifierOut, identifier->parentPath.data, true);
}

OldResult fsEntryDesc_initStruct(fsEntryDesc* desc, fsEntryDescInitArgs* args) {
    if (args->name == NULL || args->parentPath == NULL) {
        return RESULT_ERROR;
    }

    fsEntryIdentifier* identifier = &desc->identifier;
    if (fsEntryIdentifier_initStructSep(identifier, args->parentPath, args->name, args->type == FS_ENTRY_TYPE_DIRECTORY) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    desc->type              = args->type;
    if (desc->type == FS_ENTRY_TYPE_DEVICE) {
        desc->device        = args->device;
    } else {
        desc->dataRange     = args->dataRange;
    }
    desc->mode              = 0777; //TODO: Mode system stub
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
    memory_memset(desc, 0, sizeof(fsEntryDesc));
}

OldResult fsEntry_genericOpen(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags) {
    entry->desc         = desc;
    entry->openFlags    = flags;
    entry->pointer      = 0;
    entry->iNode        = iNode_open(superBlock, desc);
    if (entry->iNode == NULL) {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}

OldResult fsEntry_genericClose(SuperBlock* superBlock, fsEntry* entry) {
    if (iNode_close(entry->iNode) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    memory_memset(entry, 0, sizeof(fsEntry));

    return RESULT_SUCCESS;
}

Index64 fsEntry_genericSeek(fsEntry* entry, Index64 seekTo) {
    if (seekTo > entry->desc->dataRange.length) {
        return INVALID_INDEX;
    }
    
    return entry->pointer = seekTo;
}

OldResult fsEntry_genericRead(fsEntry* entry, void* buffer, Size n) {
    fsEntryType type = entry->desc->type;
    iNode* iNode = entry->iNode;

    BlockDevice* targetBlockDevice = iNode->superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    if (entry->desc->type == FS_ENTRY_TYPE_DEVICE) {
        targetDevice = iNode->device;
    }

    if (!device_isBlockDevice(targetDevice)) {
        charDevice_read(HOST_POINTER(targetDevice, CharDevice, device), entry->pointer, buffer, n);
        ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution
        return RESULT_SUCCESS;
    }

    void* blockBuffer = memory_allocate(POWER_2(targetDevice->granularity));
    if (blockBuffer == NULL) {
        return RESULT_ERROR;
    }

    Index64 blockIndex = DIVIDE_ROUND_DOWN_SHIFT(entry->pointer, targetDevice->granularity), offsetInBlock = entry->pointer % POWER_2(targetDevice->granularity);
    Size blockSize = POWER_2(targetDevice->granularity), remainByteNum = algorithms_min64(n, entry->desc->dataRange.length - entry->pointer);   //TODO: Or fail when access exceeds limitation?
    Range range;

    if (offsetInBlock != 0) {
        Size tmpN = 1;
        if (iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_ERROR) {
            return RESULT_ERROR;
        }

        blockDevice_readBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution

        Size byteReadN = algorithms_min64(remainByteNum, POWER_2(targetDevice->granularity) - offsetInBlock);
        memory_memcpy(buffer, blockBuffer + offsetInBlock, byteReadN);

        buffer += byteReadN;
        remainByteNum -= byteReadN;
    }

    if (remainByteNum >= blockSize) {
        Size remainBlockNum = DIVIDE_ROUND_DOWN_SHIFT(remainByteNum, targetDevice->granularity);
        OldResult res;
        while ((res = iNode_rawTranslateBlockPos(iNode, &blockIndex, &remainBlockNum, &range, 1)) == RESULT_CONTINUE) {
            blockDevice_readBlocks(targetBlockDevice, range.begin, buffer, range.length);
            ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution

            Size byteReadN = range.length * POWER_2(targetDevice->granularity);
            buffer += byteReadN;
            remainByteNum -= byteReadN;
        }

        if (res != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }
    }

    if (remainByteNum > 0) {
        Size tmpN = 1;
        if (iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_ERROR) {
            return RESULT_ERROR;
        }

        blockDevice_readBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution

        memory_memcpy(buffer, blockBuffer, remainByteNum);

        remainByteNum = 0;
    }

    memory_free(blockBuffer);

    return RESULT_SUCCESS;
    ERROR_FINAL_BEGIN(0);
    return RESULT_ERROR;    //TODO: Temporary solution
}

OldResult fsEntry_genericWrite(fsEntry* entry, const void* buffer, Size n) {
    fsEntryType type = entry->desc->type;
    iNode* iNode = entry->iNode;

    BlockDevice* targetBlockDevice = iNode->superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    if (entry->desc->type == FS_ENTRY_TYPE_DEVICE) {
        targetDevice = iNode->device;
    }

    if (!device_isBlockDevice(targetDevice)) {
        charDevice_write(HOST_POINTER(targetDevice, CharDevice, device), entry->pointer, buffer, n);
        ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution
        return RESULT_SUCCESS;
    }

    void* blockBuffer = memory_allocate(POWER_2(targetDevice->granularity));
    if (blockBuffer == NULL) {
        return RESULT_ERROR;
    }

    Index64 blockIndex = DIVIDE_ROUND_DOWN_SHIFT(entry->pointer, targetDevice->granularity), offsetInBlock = entry->pointer % POWER_2(targetDevice->granularity);
    Size blockSize = POWER_2(targetDevice->granularity), remainByteNum = n;
    if (entry->pointer + n > entry->desc->dataRange.length) {
        if (fsEntry_rawResize(entry, entry->pointer + n) != RESULT_SUCCESS) {  //TODO: May be remove this?
            return RESULT_ERROR;
        }
    }

    Range range;
    if (offsetInBlock != 0) {
        Size tmpN = 1;
        if (iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_ERROR) {
            return RESULT_ERROR;
        }

        blockDevice_readBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution

        Size byteWriteN = algorithms_min64(remainByteNum, POWER_2(targetDevice->granularity) - offsetInBlock);
        memory_memcpy(blockBuffer + offsetInBlock, buffer, byteWriteN);

        blockDevice_writeBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution

        buffer += byteWriteN;
        remainByteNum -= byteWriteN;
    }

    if (remainByteNum >= blockSize) {
        Size remainBlockNum = DIVIDE_ROUND_DOWN_SHIFT(remainByteNum, targetDevice->granularity);
        OldResult res;
        while ((res = iNode_rawTranslateBlockPos(iNode, &blockIndex, &remainBlockNum, &range, 1)) == RESULT_CONTINUE) {
            blockDevice_writeBlocks(targetBlockDevice, range.begin, buffer, range.length);
            ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution

            Size byteWriteN = range.length * POWER_2(targetDevice->granularity);
            buffer += byteWriteN;
            remainByteNum -= byteWriteN;
        }

        if (res == RESULT_ERROR) {
            return RESULT_ERROR;
        }
    }

    if (remainByteNum > 0) {
        Size tmpN = 1;
        if (iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1) == RESULT_ERROR) {
            return RESULT_ERROR;
        }

        blockDevice_readBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution

        memory_memcpy(blockBuffer, buffer, remainByteNum);

        blockDevice_writeBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0); //TODO: Temporary solution

        remainByteNum = 0;
    }

    memory_free(blockBuffer);

    return RESULT_SUCCESS;
    ERROR_FINAL_BEGIN(0);
    return RESULT_ERROR;    //TODO: Temporary solution
}

OldResult fsEntry_genericResize(fsEntry* entry, Size newSizeInByte) {
    if (iNode_isDevice(entry->iNode)) { //TODO: Temporary solution
        return RESULT_SUCCESS;
    }

    if (iNode_rawResize(entry->iNode, newSizeInByte) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    entry->desc->dataRange.length = newSizeInByte;

    return RESULT_SUCCESS;
}