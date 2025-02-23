#include<fs/devfs/inode.h>

#include<devices/block/blockDevice.h>
#include<devices/char/charDevice.h>
#include<fs/devfs/devfs.h>
#include<fs/directoryEntry.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>
#include<algorithms.h>
#include<debug.h>
#include<error.h>

static void __devfs_iNode_readData(iNode* inode, Index64 begin, void* buffer, Size byteN);

static void __devfs_iNode_writeData(iNode* inode, Index64 begin, const void* buffer, Size byteN);

static void __devfs_iNode_resize(iNode* inode, Size newSizeInByte);

static void __devfs_iNode_iterateDirectoryEntries(iNode* inode, iNodeOperationIterateDirectoryEntryFunc func, Object arg, void* ret);

static void __devfs_iNode_addDirectoryEntry(iNode* inode, ConstCstring name, fsEntryType type, iNodeAttribute* attr, ID deviceID);

static void __devfs_iNode_removeDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory);

static void __devfs_iNode_renameDirectoryEntry(iNode* inode, fsNode* entry, iNode* moveTo, ConstCstring newName);

static iNodeOperations _devfs_iNodeOperations = {
    .readData                   = __devfs_iNode_readData,
    .writeData                  = __devfs_iNode_writeData,
    .resize                     = __devfs_iNode_resize,
    .readAttr                   = iNode_genericReadAttr,
    .writeAttr                  = iNode_genericWriteAttr,
    .iterateDirectoryEntries    = __devfs_iNode_iterateDirectoryEntries,
    .addDirectoryEntry          = __devfs_iNode_addDirectoryEntry,
    .removeDirectoryEntry       = __devfs_iNode_removeDirectoryEntry,
    .renameDirectoryEntry       = __devfs_iNode_renameDirectoryEntry
};

void devfsDirectoryEntry_initStruct(DevfsDirectoryEntry* entry, ConstCstring name, fsEntryType type, Object pointTo, SuperBlock* superBlock) {
    string_initStructStr(&entry->name, name);
    ERROR_GOTO_IF_ERROR(0);
    entry->pointTo  = pointTo;
    entry->size     = type == FS_ENTRY_TYPE_DEVICE ? (Size)-1 : 0;
    entry->type     = type;
    entry->inodeID  = superBlock_allocateInodeID(superBlock);

    return;
    ERROR_FINAL_BEGIN(0);
}

iNodeOperations* devfs_iNode_getOperations() {
    return &_devfs_iNodeOperations;
}

static void __devfs_iNode_readData(iNode* inode, Index64 begin, void* buffer, Size byteN) {
    if (inode->fsNode->type == FS_ENTRY_TYPE_FILE || inode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY) {
        if (begin + byteN > inode->sizeInByte) {
            ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
        }
        DevfsInode* devfsInode = HOST_POINTER(inode, DevfsInode, inode);
        memory_memcpy(buffer, devfsInode->data + begin, byteN);
        return;
    }

    Device* targetDevice = device_getDevice(inode->deviceID);
    if (targetDevice == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (device_isBlockDevice(targetDevice)) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);   //TODO: IO to block device not supported yet
    } else {
        charDevice_read(HOST_POINTER(targetDevice, CharDevice, device), begin, buffer, byteN);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __devfs_iNode_writeData(iNode* inode, Index64 begin, const void* buffer, Size byteN) {
    if (inode->fsNode->type == FS_ENTRY_TYPE_FILE || inode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY) {
        if (begin + byteN > inode->sizeInByte) {
            ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
        }
        DevfsInode* devfsInode = HOST_POINTER(inode, DevfsInode, inode);
        memory_memcpy(devfsInode->data + begin, buffer, byteN);
        return;
    }

    Device* targetDevice = device_getDevice(inode->deviceID);
    if (targetDevice == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (device_isBlockDevice(targetDevice)) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);   //TODO: IO to block device not supported yet
    } else {
        charDevice_write(HOST_POINTER(targetDevice, CharDevice, device), begin, buffer, byteN);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __devfs_iNode_resize(iNode* inode, Size newSizeInByte) {
    if (!(inode->fsNode->type == FS_ENTRY_TYPE_FILE || inode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY)) {
        return;
    }

    void* newPages = NULL;
    
    DevfsInode* devfsInode = HOST_POINTER(inode, DevfsInode, inode);
    Size newPageNum = DIVIDE_ROUND_UP(newSizeInByte, PAGE_SIZE), oldPageNum = DIVIDE_ROUND_UP(inode->sizeInByte, PAGE_SIZE);
    
    if (newPageNum != oldPageNum) {
        if (newPageNum != 0) {
            newPages = memory_allocateFrame(newPageNum);
            if (newPages == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }
            newPages = paging_convertAddressP2V(newPages);
            
            if (newPageNum < oldPageNum) {
                memory_memcpy(newPages, devfsInode->data, newSizeInByte);
                memory_memset(newPages + newSizeInByte, 0, newPageNum * PAGE_SIZE - newSizeInByte);
            } else {
                memory_memcpy(newPages, devfsInode->data, inode->sizeInByte);
                memory_memset(newPages + inode->sizeInByte, 0, newPageNum * PAGE_SIZE - inode->sizeInByte);
            }
        }
        
        if (oldPageNum != 0) {
            memory_freeFrame(paging_convertAddressV2P(devfsInode->data));
        }

        devfsInode->data = newPages;
    }

    inode->sizeInByte = newSizeInByte;

    return;
    ERROR_FINAL_BEGIN(0);
    if (newPages != NULL) {
        memory_freeFrame(paging_convertAddressV2P(newPages));
    }
    return;
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
        iNode_rawReadData(inode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
        ERROR_GOTO_IF_ERROR(0);

        DevfsNodeMetadata* metadata = devfsSuperBlock_getMetadata(devfsSuperBlock, devfsDirectoryEntry.inodeID);
        if (metadata == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_CLEAR();
        }
        
        directoryEntry.name         = devfsDirectoryEntry.name.data;
        directoryEntry.type         = devfsDirectoryEntry.type;
        directoryEntry.inodeID      = devfsDirectoryEntry.inodeID;
        directoryEntry.mode         = DIRECTORY_ENTRY_MODE_ANY; //TODO: Support for mode
        if (fsEntryType_isDevice(directoryEntry.type)) {
            directoryEntry.size     = DIRECTORY_ENTRY_SIZE_ANY;
            directoryEntry.deviceID = (DeviceID)devfsDirectoryEntry.pointTo;
        } else {
            directoryEntry.size     = devfsDirectoryEntry.size;
            directoryEntry.deviceID = DIRECTORY_ENTRY_DEVICE_ID_ANY;
        }

        if (func(inode, &directoryEntry, arg, ret)) {
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

static void __devfs_iNode_addDirectoryEntry(iNode* inode, ConstCstring name, fsEntryType type, iNodeAttribute* attr, ID deviceID) {
    DEBUG_ASSERT_SILENT(inode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    fsNode* node = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(superBlock, DevfsSuperBlock, superBlock);

    Size currentPointer = 0;
    DevfsDirectoryEntry devfsDirectoryEntry;
    while (currentPointer < inode->sizeInByte) {
        iNode_rawReadData(inode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
        ERROR_GOTO_IF_ERROR(0);
        
        if (cstring_strcmp(name, devfsDirectoryEntry.name.data) == 0 && type == devfsDirectoryEntry.type) {
            ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
            break;
        }

        currentPointer += sizeof(DevfsDirectoryEntry);
    }
    
    bool isRealData = (type == FS_ENTRY_TYPE_FILE || type == FS_ENTRY_TYPE_DIRECTORY);
    Object pointTo = isRealData ? (Object)NULL : (Object)deviceID;
    devfsDirectoryEntry_initStruct(&devfsDirectoryEntry, name, type, pointTo, superBlock);

    iNode_rawResize(inode, inode->sizeInByte + sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    iNode_rawWriteData(inode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    node = fsNode_create(devfsDirectoryEntry.name.data, devfsDirectoryEntry.type, inode->fsNode, devfsDirectoryEntry.inodeID);
    if (node == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    devfsSuperBlock_registerMetadata(devfsSuperBlock, devfsDirectoryEntry.inodeID, node, isRealData ? 0 : INFINITE, pointTo);
    ERROR_GOTO_IF_ERROR(0);
    node = NULL;

    return;
    ERROR_FINAL_BEGIN(0);
    if (node != NULL) {
        fsNode_release(node);
    }
}

static void __devfs_iNode_removeDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory) {
    DEBUG_ASSERT_SILENT(inode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    void* remainingData = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    DevfsSuperBlock* devfsSuperBlock = HOST_POINTER(superBlock, DevfsSuperBlock, superBlock);

    Size currentPointer = 0;
    bool found = false;
    DevfsDirectoryEntry devfsDirectoryEntry;
    while (currentPointer < inode->sizeInByte) {
        iNode_rawReadData(inode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
        ERROR_GOTO_IF_ERROR(0);
        
        if (cstring_strcmp(name, devfsDirectoryEntry.name.data) == 0 && isDirectory == (devfsDirectoryEntry.type == FS_ENTRY_TYPE_DIRECTORY)) {
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
        iNode_rawReadData(inode, currentPointer, remainingData + sizeof(DevfsDirectoryEntry), remainingDataSize);
        ERROR_GOTO_IF_ERROR(0);
        iNode_rawWriteData(inode, currentPointer, remainingData, remainingDataSize);
        ERROR_GOTO_IF_ERROR(0);
        memory_free(remainingData);
        remainingData = NULL;
    }

    iNode_rawResize(inode, inode->sizeInByte - sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
    if (remainingData != NULL) {
        memory_free(remainingData);
    }
}

static void __devfs_iNode_renameDirectoryEntry(iNode* inode, fsNode* entry, iNode* moveTo, ConstCstring newName) {
    DEBUG_ASSERT_SILENT(inode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(moveTo->fsNode->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(inode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    void* remainingData = NULL;

    //Remove from inode directory
    
    Size currentPointer = 0;
    bool found = false;
    DevfsDirectoryEntry devfsDirectoryEntry, transplantDirEntry;
    while (currentPointer < inode->sizeInByte) {
        iNode_rawReadData(inode, currentPointer, &transplantDirEntry, sizeof(DevfsDirectoryEntry));
        ERROR_GOTO_IF_ERROR(0);

        if (cstring_strcmp(entry->name.data, transplantDirEntry.name.data) == 0 && entry->type == transplantDirEntry.type) {
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
        iNode_rawReadData(inode, currentPointer, remainingData + sizeof(DevfsDirectoryEntry), remainingDataSize);
        ERROR_GOTO_IF_ERROR(0);
        iNode_rawWriteData(inode, currentPointer, remainingData, remainingDataSize);
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
        iNode_rawReadData(moveTo, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
        ERROR_GOTO_IF_ERROR(0);

        if (cstring_strcmp(entry->name.data, devfsDirectoryEntry.name.data) == 0 && entry->type == transplantDirEntry.type) {
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

    iNode_rawWriteData(moveTo, currentPointer, &transplantDirEntry, sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    fsNode_transplant(entry, moveTo->fsNode);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);

    if (remainingData != NULL) {
        memory_free(remainingData);
    }
}