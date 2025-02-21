#include<fs/devfs/inode.h>

#include<devices/block/blockDevice.h>
#include<devices/char/charDevice.h>
#include<fs/devfs/blockChain.h>
#include<fs/devfs/devfs.h>
#include<fs/directoryEntry.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>
#include<algorithms.h>
#include<debug.h>
#include<error.h>

static void __devfs_iNode_readData(iNode* inode, Index64 begin, void* buffer, Size byteN);

static void __devfs_iNode_writeData(iNode* inode, Index64 begin, const void* buffer, Size byteN);

static void __devfs_iNode_doReadData(iNode* inode, Index64 begin, void* buffer, Size byteN, void* blockBuffer);

static void __devfs_iNode_doWriteData(iNode* inode, Index64 begin, const void* buffer, Size byteN, void* blockBuffer);

static void __devfs_iNode_resize(iNode* inode, Size newSizeInByte);

static void __devfs_iNode_iterateDirectoryEntries(iNode* inode, iNodeOperationIterateDirectoryEntryFunc func, Object arg, void* ret);

static fsEntryType __devfs_iNode_getDirectoryEntryType(DevfsDirectoryEntry* entry);

static void __devfs_iNode_addDirectoryEntry(iNode* inode, ConstCstring name, fsEntryType type, iNodeAttribute* attr, ID deviceID);

static void __devfs_iNode_removeDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory);

static void __devfs_iNode_renameDirectoryEntry(iNode* inode, fsNode* entry, iNode* moveTo, ConstCstring newName);

static fsNode* __devfs_directoryEntryToFSnode(iNode* inode, DevfsDirectoryEntry* devfsDirectoryEntry);

static iNodeOperations _devfs_iNodeOperations = {
    .readData               = __devfs_iNode_readData,
    .writeData              = __devfs_iNode_writeData,
    .resize                 = __devfs_iNode_resize,
    .readAttr               = iNode_genericReadAttr,
    .writeAttr              = iNode_genericWriteAttr,
    .addDirectoryEntry      = __devfs_iNode_addDirectoryEntry,
    .removeDirectoryEntry   = __devfs_iNode_removeDirectoryEntry,
    .renameDirectoryEntry   = __devfs_iNode_renameDirectoryEntry
};

void devfsDirectoryEntry_initStruct(DevfsDirectoryEntry* entry, SuperBlock* superBlock, ConstCstring name, fsEntryType type, ID deviceID) {
    if (cstring_strlen(name) > DEVFS_DIRECTORY_ENTRY_NAME_LIMIT) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    cstring_strcpy(entry->name, name);
    if (type == FS_ENTRY_TYPE_DEVICE) {
        entry->device = deviceID;
    } else {
        entry->dataRange = (Range) {
            .begin = INVALID_INDEX,
            .length = 0
        };
    }

    entry->attribute = EMPTY_FLAGS;
    switch (type) {
        case FS_ENTRY_TYPE_DIRECTORY: {
            SET_FLAG_BACK(entry->attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY);
            break;
        }
        case FS_ENTRY_TYPE_FILE: {
            SET_FLAG_BACK(entry->attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_FILE);
            break;
        }
        case FS_ENTRY_TYPE_DEVICE: {
            SET_FLAG_BACK(entry->attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE);
            break;
        }
        default: {
            ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
        }
    }
    
    entry->magic = __DEVFS_DIRECTORY_ENTRY_MAGIC;
    entry->inodeID = superBlock_allocateInodeID(superBlock);

    return;
    ERROR_FINAL_BEGIN(0);
}

iNodeOperations* devfs_iNode_getOperations() {
    return &_devfs_iNodeOperations;
}

static void __devfs_iNode_readData(iNode* inode, Index64 begin, void* buffer, Size byteN) {
    void* blockBuffer = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    blockBuffer = memory_allocate(POWER_2(targetDevice->granularity));
    if (blockBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    __devfs_iNode_doReadData(inode, begin, buffer, byteN, blockBuffer);
    ERROR_GOTO_IF_ERROR(0);

    memory_free(blockBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        memory_free(blockBuffer);
    }
}

static void __devfs_iNode_writeData(iNode* inode, Index64 begin, const void* buffer, Size byteN) {
    void* blockBuffer = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    blockBuffer = memory_allocate(POWER_2(targetDevice->granularity));
    if (blockBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    __devfs_iNode_doWriteData(inode, begin, buffer, byteN, blockBuffer);
    ERROR_GOTO_IF_ERROR(0);

    memory_free(blockBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        memory_free(blockBuffer);
    }
}

static void __devfs_iNode_doReadData(iNode* inode, Index64 begin, void* buffer, Size byteN, void* blockBuffer) {
    SuperBlock* superBlock = inode->superBlock;
    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(superBlock, DevfsSuperBlock, superBlock);
    DevfsBlockChains* blockChains = &devfsSuperBlock->blockChains;
    
    fsNode* node = inode->fsNode;
    DevfsInode* devfsInode = HOST_POINTER(inode, DevfsInode, inode);

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    if (node->type == FS_ENTRY_TYPE_DEVICE) {
        targetDevice = device_getDevice(node->inodeID);
        if (targetDevice == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
    }

    if (!device_isBlockDevice(targetDevice)) {
        charDevice_read(HOST_POINTER(targetDevice, CharDevice, device), begin, buffer, byteN);
        ERROR_GOTO_IF_ERROR(0);
        return;
    }

    Size blockSize = POWER_2(targetDevice->granularity);

    Index64 currentBlockIndex = devfs_blockChain_get(blockChains, devfsInode->firstBlock, begin / blockSize);
    if (currentBlockIndex == INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size remainByteNum = algorithms_umin64(byteN, inode->sizeInByte - begin);   //TODO: Or fail when access exceeds limitation?

    if (begin % blockSize != 0) {
        Index64 offsetInBlock = begin % blockSize;
        blockDevice_readBlocks(targetBlockDevice, currentBlockIndex, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        Size byteReadN = algorithms_umin64(remainByteNum, blockSize - offsetInBlock);
        memory_memcpy(buffer, blockBuffer + offsetInBlock, byteReadN);
        
        currentBlockIndex = devfs_blockChain_get(blockChains, currentBlockIndex, 1);

        buffer += byteReadN;
        remainByteNum -= byteReadN;
    }

    if (remainByteNum >= blockSize) {
        Size remainingFullBlockNum = remainByteNum / blockSize;
        while (remainingFullBlockNum > 0) {
            DEBUG_ASSERT_SILENT(currentBlockIndex != INVALID_INDEX);

            Size continousBlockLength = 0;
            currentBlockIndex = devfs_blockChain_step(blockChains, currentBlockIndex, remainingFullBlockNum, &continousBlockLength);
            DEBUG_ASSERT_SILENT(VALUE_WITHIN(0, remainingFullBlockNum, continousBlockLength, <, <=));

            blockDevice_readBlocks(targetBlockDevice, currentBlockIndex, buffer, continousBlockLength);
            ERROR_GOTO_IF_ERROR(0);

            buffer += continousBlockLength * blockSize;
            remainingFullBlockNum -= continousBlockLength;
        }

        remainByteNum %= blockSize;
    }

    if (remainByteNum > 0) {
        DEBUG_ASSERT_SILENT(currentBlockIndex != INVALID_INDEX);
        blockDevice_readBlocks(targetBlockDevice, currentBlockIndex, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        memory_memcpy(buffer, blockBuffer, remainByteNum);

        remainByteNum = 0;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __devfs_iNode_doWriteData(iNode* inode, Index64 begin, const void* buffer, Size byteN, void* blockBuffer) {
    SuperBlock* superBlock = inode->superBlock;
    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(superBlock, DevfsSuperBlock, superBlock);
    DevfsBlockChains* blockChains = &devfsSuperBlock->blockChains;
    
    fsNode* node = inode->fsNode;
    DevfsInode* devfsInode = HOST_POINTER(inode, DevfsInode, inode);

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    if (node->type == FS_ENTRY_TYPE_DEVICE) {
        targetDevice = device_getDevice(node->inodeID);
        if (targetDevice == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
    }

    if (!device_isBlockDevice(targetDevice)) {
        charDevice_write(HOST_POINTER(targetDevice, CharDevice, device), begin, buffer, byteN);
        ERROR_GOTO_IF_ERROR(0);
        return;
    }

    Size blockSize = POWER_2(targetDevice->granularity);

    Index64 currentBlockIndex = devfs_blockChain_get(blockChains, devfsInode->firstBlock, begin / blockSize);
    if (currentBlockIndex == INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size remainByteNum = algorithms_umin64(byteN, inode->sizeInByte - begin);   //TODO: Or fail when access exceeds limitation?

    if (begin % blockSize != 0) {
        Index64 offsetInBlock = begin % blockSize;
        blockDevice_readBlocks(targetBlockDevice, currentBlockIndex, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);
        
        Size byteReadN = algorithms_umin64(remainByteNum, blockSize - offsetInBlock);
        memory_memcpy(blockBuffer + offsetInBlock, buffer, byteReadN);
        
        blockDevice_writeBlocks(targetBlockDevice, currentBlockIndex, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        currentBlockIndex = devfs_blockChain_get(blockChains, currentBlockIndex, 1);

        buffer += byteReadN;
        remainByteNum -= byteReadN;
    }

    if (remainByteNum >= blockSize) {
        Size remainingFullBlockNum = remainByteNum / blockSize;
        while (remainingFullBlockNum > 0) {
            DEBUG_ASSERT_SILENT(currentBlockIndex != INVALID_INDEX);

            Size continousBlockLength = 0;
            Index8 nextCurrentBlockIndex = devfs_blockChain_step(blockChains, currentBlockIndex, remainingFullBlockNum, &continousBlockLength);
            DEBUG_ASSERT_SILENT(VALUE_WITHIN(0, remainingFullBlockNum, continousBlockLength, <, <=));

            blockDevice_writeBlocks(targetBlockDevice, currentBlockIndex, buffer, continousBlockLength);
            ERROR_GOTO_IF_ERROR(0);

            currentBlockIndex = nextCurrentBlockIndex;

            buffer += continousBlockLength * blockSize;
            remainingFullBlockNum -= continousBlockLength;
        }

        remainByteNum %= blockSize;
    }

    if (remainByteNum > 0) {
        DEBUG_ASSERT_SILENT(currentBlockIndex != INVALID_INDEX);
        blockDevice_readBlocks(targetBlockDevice, currentBlockIndex, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);
        
        memory_memcpy(blockBuffer, buffer, remainByteNum);
        
        blockDevice_writeBlocks(targetBlockDevice, currentBlockIndex, blockBuffer, 1);
        ERROR_GOTO_IF_ERROR(0);

        remainByteNum = 0;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __devfs_iNode_resize(iNode* inode, Size newSizeInByte) {
    Index32 freeBlockChain = INVALID_INDEX;
    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(inode->superBlock, DevfsSuperBlock, superBlock);
    DevfsInode* devfsInode = HOST_POINTER(inode, DevfsInode, inode);

    DevfsBlockChains* blockChains = &devfsSuperBlock->blockChains;
    Size newSizeInBlock = DIVIDE_ROUND_UP_SHIFT(newSizeInByte, inode->superBlock->blockDevice->device.granularity), oldSizeInBlock = inode->sizeInBlock;
    DEBUG_ASSERT_SILENT((oldSizeInBlock == 0) ^ (devfsInode->firstBlock != (Index8)INVALID_INDEX));

    if (newSizeInBlock < oldSizeInBlock) {
        if (newSizeInBlock == 0) {
            devfs_blockChain_freeChain(blockChains, devfsInode->firstBlock);
            devfsInode->firstBlock = INVALID_INDEX;
        } else {
            Index32 tail = devfs_blockChain_get(blockChains, devfsInode->firstBlock, newSizeInBlock - 1);
            if (tail == INVALID_INDEX) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }
    
            Index32 cut = devfs_blockChain_cutChain(blockChains, tail);
            devfs_blockChain_freeChain(blockChains, cut);
        }
    } else if (newSizeInBlock > oldSizeInBlock) {
        freeBlockChain = devfs_blockChain_allocChain(blockChains, newSizeInBlock - oldSizeInBlock);
        if (freeBlockChain == INVALID_INDEX) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        if (oldSizeInBlock == 0) {
            devfsInode->firstBlock = freeBlockChain;
        } else {
            Index32 tail = devfs_blockChain_get(blockChains, devfsInode->firstBlock, oldSizeInBlock - 1);
            if (freeBlockChain == INVALID_INDEX) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }
    
            devfs_blockChain_insertChain(blockChains, tail, freeBlockChain);
        }
    }

    inode->sizeInBlock = newSizeInBlock;
    inode->sizeInByte = newSizeInByte;

    return;
    ERROR_FINAL_BEGIN(0);
    if (freeBlockChain != INVALID_INDEX) {
        devfs_blockChain_freeChain(blockChains, freeBlockChain);
    }
}

static void __devfs_iNode_iterateDirectoryEntries(iNode* inode, iNodeOperationIterateDirectoryEntryFunc func, Object arg, void* ret) {
    DEBUG_ASSERT_SILENT(inode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    void* blockBuffer = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(superBlock, DevfsSuperBlock, superBlock);
    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    Size blockSize = POWER_2(targetDevice->granularity);
    
    blockBuffer = memory_allocate(blockSize);
    if (blockBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size currentPointer = 0;
    DevfsDirectoryEntry devfsDirectoryEntry;
    DirectoryEntry directoryEntry;
    while (currentPointer < inode->sizeInByte) {
        __devfs_iNode_doReadData(inode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry), blockBuffer);
        ERROR_GOTO_IF_ERROR(0);

        DevfsNodeMetadata* metadata = devfsSuperBlock_getMetadata(devfsSuperBlock, devfsDirectoryEntry.inodeID);
        if (metadata == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_CLEAR();
        }
        
        directoryEntry.name = devfsDirectoryEntry.name;
        directoryEntry.type = __devfs_iNode_getDirectoryEntryType(&devfsDirectoryEntry);
        directoryEntry.inodeID = devfsDirectoryEntry.inodeID;
        if (fsEntryType_isDevice(directoryEntry.type)) {
            directoryEntry.size = DIRECTORY_ENTRY_SIZE_ANY;
            directoryEntry.deviceID = devfsDirectoryEntry.device;
        } else {
            directoryEntry.size = devfsDirectoryEntry.dataRange.length;
            directoryEntry.deviceID = DIRECTORY_ENTRY_DEVICE_ID_ANY;
        }

        if (func(&directoryEntry, arg, ret)) {
            break;
        }
        ERROR_GOTO_IF_ERROR(0);
        
        currentPointer += sizeof(DevfsDirectoryEntry);
    }

    memory_free(blockBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        memory_free(blockBuffer);
    }

    return;
}

static fsEntryType __devfs_iNode_getDirectoryEntryType(DevfsDirectoryEntry* entry) {
    if (TEST_FLAGS(entry->attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
        return FS_ENTRY_TYPE_DIRECTORY;
    } else if (TEST_FLAGS(entry->attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_FILE)) {
        return FS_ENTRY_TYPE_FILE;
    } else if (TEST_FLAGS(entry->attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE)) {
        return FS_ENTRY_TYPE_DEVICE;
    }

    return FS_ENTRY_TYPE_DUMMY;
}

static void __devfs_iNode_addDirectoryEntry(iNode* inode, ConstCstring name, fsEntryType type, iNodeAttribute* attr, ID deviceID) {
    DEBUG_ASSERT_SILENT(inode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    void* blockBuffer = NULL;
    fsNode* node = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(superBlock, DevfsSuperBlock, superBlock);
    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    Size blockSize = POWER_2(targetDevice->granularity);
    
    blockBuffer = memory_allocate(blockSize);
    if (blockBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size currentPointer = 0;
    DevfsDirectoryEntry devfsDirectoryEntry;
    while (currentPointer < inode->sizeInByte) {
        __devfs_iNode_doReadData(inode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry), blockBuffer);
        ERROR_GOTO_IF_ERROR(0);
        
        Uint8 attribute = devfsDirectoryEntry.attribute;
        if (cstring_strcmp(name, devfsDirectoryEntry.name) == 0 && (type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
            break;
        }

        currentPointer += sizeof(DevfsDirectoryEntry);
    }

    devfsDirectoryEntry_initStruct(&devfsDirectoryEntry, superBlock, name, type, deviceID);

    iNode_rawResize(inode, inode->sizeInByte + sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    __devfs_iNode_doWriteData(inode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry), blockBuffer);
    ERROR_GOTO_IF_ERROR(0);

    node = __devfs_directoryEntryToFSnode(inode, &devfsDirectoryEntry);
    if (node == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    bool isDevice = TEST_FLAGS(devfsDirectoryEntry.attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY);
    devfsSuperBlock_registerMetadata(devfsSuperBlock, devfsDirectoryEntry.inodeID, node, isDevice ? INFINITE : devfsDirectoryEntry.dataRange.length, isDevice ? devfsDirectoryEntry.device : devfsDirectoryEntry.dataRange.begin);
    ERROR_GOTO_IF_ERROR(0);
    node = NULL;

    memory_free(blockBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        memory_free(blockBuffer);
    }

    if (node != NULL) {
        fsNode_release(node);
    }
}

static void __devfs_iNode_removeDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory) {
    DEBUG_ASSERT_SILENT(inode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    void* blockBuffer = NULL, * remainingData = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(superBlock, DevfsSuperBlock, superBlock);
    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    Size blockSize = POWER_2(targetDevice->granularity);
    
    blockBuffer = memory_allocate(blockSize);
    if (blockBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size currentPointer = 0;
    bool found = false;
    DevfsDirectoryEntry devfsDirectoryEntry;
    while (currentPointer < inode->sizeInByte) {
        __devfs_iNode_doReadData(inode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry), blockBuffer);
        ERROR_GOTO_IF_ERROR(0);

        Uint8 attribute = devfsDirectoryEntry.attribute;
        if (cstring_strcmp(name, devfsDirectoryEntry.name) == 0 && isDirectory == TEST_FLAGS(attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += sizeof(DevfsDirectoryEntry);
    }

    if (!found) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    devfsSuperBlock_unregisterMetadata(devfsSuperBlock, devfsDirectoryEntry.inodeID);
    ERROR_GOTO_IF_ERROR(0);

    if (currentPointer + sizeof(DevfsDirectoryEntry) < inode->sizeInByte) {
        Size remainingDataSize = inode->sizeInByte - currentPointer - sizeof(DevfsDirectoryEntry);
        remainingData = memory_allocate(remainingDataSize);
        __devfs_iNode_doReadData(inode, currentPointer, remainingData + sizeof(DevfsDirectoryEntry), remainingDataSize, blockBuffer);
        ERROR_GOTO_IF_ERROR(0);
        __devfs_iNode_doWriteData(inode, currentPointer, remainingData, remainingDataSize, blockBuffer);
        ERROR_GOTO_IF_ERROR(0);
        memory_free(remainingData);
        remainingData = NULL;
    }

    iNode_rawResize(inode, inode->sizeInByte - sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    memory_free(blockBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        memory_free(blockBuffer);
    }

    if (remainingData != NULL) {
        memory_free(remainingData);
    }
}

static void __devfs_iNode_renameDirectoryEntry(iNode* inode, fsNode* entry, iNode* moveTo, ConstCstring newName) {
    DEBUG_ASSERT_SILENT(inode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(moveTo->fsNode->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(inode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    void* blockBuffer = NULL, * remainingData = NULL;

    SuperBlock* superBlock = inode->superBlock;
    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    Size blockSize = POWER_2(targetDevice->granularity);
    
    blockBuffer = memory_allocate(blockSize);
    if (blockBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    //Remove from inode directory
    
    Size currentPointer = 0;
    bool found = false;
    DevfsDirectoryEntry devfsDirectoryEntry, transplantDirEntry;
    while (currentPointer < inode->sizeInByte) {
        __devfs_iNode_doReadData(inode, currentPointer, &transplantDirEntry, sizeof(DevfsDirectoryEntry), blockBuffer);
        ERROR_GOTO_IF_ERROR(0);

        Uint8 attribute = transplantDirEntry.attribute;
        if (cstring_strcmp(entry->name.data, transplantDirEntry.name) == 0 && (entry->type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += sizeof(DevfsDirectoryEntry);
    }

    if (!found) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    if (currentPointer + sizeof(DevfsDirectoryEntry) < inode->sizeInByte) {
        Size remainingDataSize = inode->sizeInByte - currentPointer - sizeof(DevfsDirectoryEntry);
        remainingData = memory_allocate(remainingDataSize);
        __devfs_iNode_doReadData(inode, currentPointer, remainingData + sizeof(DevfsDirectoryEntry), remainingDataSize, blockBuffer);
        ERROR_GOTO_IF_ERROR(0);
        __devfs_iNode_doWriteData(inode, currentPointer, remainingData, remainingDataSize, blockBuffer);
        ERROR_GOTO_IF_ERROR(0);
        memory_free(remainingData);
        remainingData = NULL;
    }

    iNode_rawResize(inode, inode->sizeInByte - sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    //Write to moveTo directory

    currentPointer = 0;
    found = false;
    while (currentPointer < moveTo->sizeInByte) {
        __devfs_iNode_doReadData(moveTo, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry), blockBuffer);
        ERROR_GOTO_IF_ERROR(0);

        Uint8 attribute = devfsDirectoryEntry.attribute;
        if (cstring_strcmp(entry->name.data, devfsDirectoryEntry.name) == 0 && (entry->type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += sizeof(DevfsDirectoryEntry);
    }

    if (found) {
        ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
    }
    
    iNode_rawResize(moveTo, moveTo->sizeInByte + sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    __devfs_iNode_doWriteData(moveTo, currentPointer, &transplantDirEntry, sizeof(DevfsDirectoryEntry), blockBuffer);
    ERROR_GOTO_IF_ERROR(0);

    fsNode_transplant(entry, moveTo->fsNode);
    ERROR_GOTO_IF_ERROR(0);

    memory_free(blockBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        memory_free(blockBuffer);
    }

    if (remainingData != NULL) {
        memory_free(remainingData);
    }
}

static fsNode* __devfs_directoryEntryToFSnode(iNode* inode, DevfsDirectoryEntry* devfsDirectoryEntry) {
    if (devfsDirectoryEntry->magic != __DEVFS_DIRECTORY_ENTRY_MAGIC) {
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    fsNode* ret = NULL;

    Uint8 attribute = devfsDirectoryEntry->attribute;
    if (TEST_FLAGS(attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_FILE)) {
        ret = fsNode_create(devfsDirectoryEntry->name, FS_ENTRY_TYPE_FILE, inode->fsNode);
    } else if (TEST_FLAGS(attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
        ret = fsNode_create(devfsDirectoryEntry->name, FS_ENTRY_TYPE_DIRECTORY, inode->fsNode);
    } else if (TEST_FLAGS(attribute, __DEVFS_DIRECTORY_ENTRY_ATTRIBUTE_DEVICE)) {
        ret = fsNode_create(devfsDirectoryEntry->name, FS_ENTRY_TYPE_FILE, inode->fsNode);
    }

    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    ret->inodeID = devfsDirectoryEntry->inodeID;    //TODO: Ugly code

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}