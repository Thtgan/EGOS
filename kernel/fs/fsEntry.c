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

void fsEntryIdentifier_initStruct(fsEntryIdentifier* identifier, ConstCstring path, bool isDirectory) {
    if (*path == '\0') {
        string_initStruct(&identifier->parentPath, "");
        ERROR_GOTO_IF_ERROR(0);
        string_initStruct(&identifier->name, "");
        ERROR_GOTO_IF_ERROR(0);
    } else {
        ConstCstring sep = cstring_strrchr(path, FS_PATH_SEPERATOR);
        if (sep == NULL) {
            ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
        }
        string_initStructN(&identifier->parentPath, path, ARRAY_POINTER_TO_INDEX(path, sep));
        ERROR_GOTO_IF_ERROR(0);
        string_initStruct(&identifier->name, sep + 1);
        ERROR_GOTO_IF_ERROR(0);
    }

    identifier->isDirectory = isDirectory;

    return;
    ERROR_FINAL_BEGIN(0);
    if (STRING_IS_AVAILABLE(&identifier->parentPath)) {
        string_clearStruct(&identifier->parentPath);
    }

    if (STRING_IS_AVAILABLE(&identifier->name)) {
        string_clearStruct(&identifier->name);
    }
}

void fsEntryIdentifier_initStructSep(fsEntryIdentifier* identifier, ConstCstring parentPath, ConstCstring name, bool isDirectory) {
    string_initStruct(&identifier->parentPath, parentPath);
    ERROR_GOTO_IF_ERROR(0);
    string_initStruct(&identifier->name, name);
    ERROR_GOTO_IF_ERROR(0);

    identifier->isDirectory = isDirectory;

    return;
    ERROR_FINAL_BEGIN(0);
    if (STRING_IS_AVAILABLE(&identifier->parentPath)) {
        string_clearStruct(&identifier->parentPath);
    }

    if (STRING_IS_AVAILABLE(&identifier->name)) {
        string_clearStruct(&identifier->name);
    }
}

void fsEntryIdentifier_initStructSepN(fsEntryIdentifier* identifier, ConstCstring parentPath, Size n1, ConstCstring name, Size n2, bool isDirectory) {
    string_initStructN(&identifier->parentPath, parentPath, n1);
    ERROR_GOTO_IF_ERROR(0);
    string_initStructN(&identifier->name, name, n2);
    ERROR_GOTO_IF_ERROR(0);

    identifier->isDirectory = isDirectory;

    return;
    ERROR_FINAL_BEGIN(0);
    if (STRING_IS_AVAILABLE(&identifier->parentPath)) {
        string_clearStruct(&identifier->parentPath);
    }

    if (STRING_IS_AVAILABLE(&identifier->name)) {
        string_clearStruct(&identifier->name);
    }
}

void fsEntryIdentifier_clearStruct(fsEntryIdentifier* identifier) {
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(identifier));
    string_clearStruct(&identifier->parentPath);
    string_clearStruct(&identifier->name);
    identifier->isDirectory = false;
}

void fsEntryIdentifier_copy(fsEntryIdentifier* des, fsEntryIdentifier* src) {
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(des));
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(src));
    string_copy(&des->name, &src->name);
    ERROR_GOTO_IF_ERROR(0);
    string_copy(&des->parentPath, &src->parentPath);
    ERROR_GOTO_IF_ERROR(0);
    
    des->isDirectory = src->isDirectory;

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsEntryIdentifier_getParent(fsEntryIdentifier* identifier, fsEntryIdentifier* parentIdentifierOut) {
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(identifier));
    fsEntryIdentifier_initStruct(parentIdentifierOut, identifier->parentPath.data, true);
}

void fsEntryDesc_initStruct(fsEntryDesc* desc, fsEntryDescInitArgs* args) {
    if (args->name == NULL || args->parentPath == NULL) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    fsEntryIdentifier* identifier = &desc->identifier;
    fsEntryIdentifier_initStructSep(identifier, args->parentPath, args->name, args->type == FS_ENTRY_TYPE_DIRECTORY);
    ERROR_GOTO_IF_ERROR(0);

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

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsEntryDesc_clearStruct(fsEntryDesc* desc) {
    fsEntryIdentifier_clearStruct(&desc->identifier);
    memory_memset(desc, 0, sizeof(fsEntryDesc));
}

void fsEntry_genericOpen(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags) {
    entry->desc         = desc;
    entry->openFlags    = flags;
    entry->pointer      = 0;
    entry->iNode        = iNode_open(superBlock, desc);
    if (entry->iNode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsEntry_genericClose(SuperBlock* superBlock, fsEntry* entry) {
    iNode_close(entry->iNode);
    ERROR_GOTO_IF_ERROR(0);

    memory_memset(entry, 0, sizeof(fsEntry));

    return;
    ERROR_FINAL_BEGIN(0);
}

Index64 fsEntry_genericSeek(fsEntry* entry, Index64 seekTo) {
    if (seekTo > entry->desc->dataRange.length) {
        return INVALID_INDEX;
    }
    
    return entry->pointer = seekTo;
}

void fsEntry_genericRead(fsEntry* entry, void* buffer, Size n) {
    void* blockBuffer = NULL;
    
    fsEntryType type = entry->desc->type;
    iNode* iNode = entry->iNode;

    BlockDevice* targetBlockDevice = iNode->superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    if (entry->desc->type == FS_ENTRY_TYPE_DEVICE) {
        targetDevice = iNode->device;
    }

    if (!device_isBlockDevice(targetDevice)) {
        charDevice_read(HOST_POINTER(targetDevice, CharDevice, device), entry->pointer, buffer, n);
        ERROR_GOTO_IF_ERROR(0);
        return;
    }

    blockBuffer = memory_allocate(POWER_2(targetDevice->granularity));
    if (blockBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Index64 blockIndex = DIVIDE_ROUND_DOWN_SHIFT(entry->pointer, targetDevice->granularity), offsetInBlock = entry->pointer % POWER_2(targetDevice->granularity);
    Size blockSize = POWER_2(targetDevice->granularity), remainByteNum = algorithms_min64(n, entry->desc->dataRange.length - entry->pointer);   //TODO: Or fail when access exceeds limitation?
    Range range;

    if (offsetInBlock != 0) {
        Size tmpN = 1;
        iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1);
        ERROR_GOTO_IF_ERROR(0);

        blockDevice_readBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        Size byteReadN = algorithms_min64(remainByteNum, POWER_2(targetDevice->granularity) - offsetInBlock);
        memory_memcpy(buffer, blockBuffer + offsetInBlock, byteReadN);

        buffer += byteReadN;
        remainByteNum -= byteReadN;
    }

    if (remainByteNum >= blockSize) {
        Size remainBlockNum = DIVIDE_ROUND_DOWN_SHIFT(remainByteNum, targetDevice->granularity);
        while (true) {
            bool isEnd = iNode_rawTranslateBlockPos(iNode, &blockIndex, &remainBlockNum, &range, 1);
            ERROR_GOTO_IF_ERROR(0);
            if (isEnd) {
                break;
            }

            blockDevice_readBlocks(targetBlockDevice, range.begin, buffer, range.length);
            ERROR_GOTO_IF_ERROR(0);

            Size byteReadN = range.length * POWER_2(targetDevice->granularity);
            buffer += byteReadN;
            remainByteNum -= byteReadN;
        }
    }

    if (remainByteNum > 0) {
        Size tmpN = 1;
        iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1);
        ERROR_GOTO_IF_ERROR(0);

        blockDevice_readBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        memory_memcpy(buffer, blockBuffer, remainByteNum);

        remainByteNum = 0;
    }

    memory_free(blockBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        memory_free(blockBuffer);
    }
}

void fsEntry_genericWrite(fsEntry* entry, const void* buffer, Size n) {
    void* blockBuffer = NULL;

    fsEntryType type = entry->desc->type;
    iNode* iNode = entry->iNode;

    BlockDevice* targetBlockDevice = iNode->superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    if (entry->desc->type == FS_ENTRY_TYPE_DEVICE) {
        targetDevice = iNode->device;
    }

    if (!device_isBlockDevice(targetDevice)) {
        charDevice_write(HOST_POINTER(targetDevice, CharDevice, device), entry->pointer, buffer, n);
        ERROR_GOTO_IF_ERROR(0);
        return;
    }

    blockBuffer = memory_allocate(POWER_2(targetDevice->granularity));
    if (blockBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Index64 blockIndex = DIVIDE_ROUND_DOWN_SHIFT(entry->pointer, targetDevice->granularity), offsetInBlock = entry->pointer % POWER_2(targetDevice->granularity);
    Size blockSize = POWER_2(targetDevice->granularity), remainByteNum = n;
    if (entry->pointer + n > entry->desc->dataRange.length) {
        fsEntry_rawResize(entry, entry->pointer + n);
        ERROR_GOTO_IF_ERROR(0);
    }

    Range range;
    if (offsetInBlock != 0) {
        Size tmpN = 1;
        iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1);
        ERROR_GOTO_IF_ERROR(0);

        blockDevice_readBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        Size byteWriteN = algorithms_min64(remainByteNum, POWER_2(targetDevice->granularity) - offsetInBlock);
        memory_memcpy(blockBuffer + offsetInBlock, buffer, byteWriteN);

        blockDevice_writeBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        buffer += byteWriteN;
        remainByteNum -= byteWriteN;
    }

    if (remainByteNum >= blockSize) {
        Size remainBlockNum = DIVIDE_ROUND_DOWN_SHIFT(remainByteNum, targetDevice->granularity);
        while (true) {
            bool isEnd = iNode_rawTranslateBlockPos(iNode, &blockIndex, &remainBlockNum, &range, 1);
            ERROR_GOTO_IF_ERROR(0);
            if (isEnd) {
                break;
            }

            blockDevice_writeBlocks(targetBlockDevice, range.begin, buffer, range.length);
            ERROR_GOTO_IF_ERROR(0);

            Size byteReadN = range.length * POWER_2(targetDevice->granularity);
            buffer += byteReadN;
            remainByteNum -= byteReadN;
        }
    }

    if (remainByteNum > 0) {
        Size tmpN = 1;
        iNode_rawTranslateBlockPos(iNode, &blockIndex, &tmpN, &range, 1);
        ERROR_GOTO_IF_ERROR(0);

        blockDevice_readBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        memory_memcpy(blockBuffer, buffer, remainByteNum);

        blockDevice_writeBlocks(targetBlockDevice, range.begin, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        remainByteNum = 0;
    }

    memory_free(blockBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        memory_free(blockBuffer);
    }
}

void fsEntry_genericResize(fsEntry* entry, Size newSizeInByte) {
    if (iNode_isDevice(entry->iNode)) { //TODO: Temporary solution
        return;
    }

    iNode_rawResize(entry->iNode, newSizeInByte);
    ERROR_GOTO_IF_ERROR(0);

    entry->desc->dataRange.length = newSizeInByte;

    return;
    ERROR_FINAL_BEGIN(0);
}