#include<fs/devfs/vnode.h>

#include<devices/device.h>
#include<fs/devfs/devfs.h>
#include<fs/directoryEntry.h>
#include<fs/vnode.h>
#include<fs/fscore.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>
#include<algorithms.h>
#include<debug.h>
#include<error.h>

static void __devfs_vNode_readData(vNode* vnode, Index64 begin, void* buffer, Size byteN);

static void __devfs_vNode_writeData(vNode* vnode, Index64 begin, const void* buffer, Size byteN);

static void __devfs_vNode_resize(vNode* vnode, Size newSizeInByte);

static Index64 __devfs_vNode_addDirectoryEntry(vNode* vnode, ConstCstring name, fsEntryType type, vNodeAttribute* attr, ID deviceID);

static void __devfs_vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory);

static void __devfs_vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName);

static void __devfs_vNode_readDirectoryEntries(vNode* vnode);

static vNodeOperations _devfs_vNodeOperations = {
    .readData                   = __devfs_vNode_readData,
    .writeData                  = __devfs_vNode_writeData,
    .resize                     = __devfs_vNode_resize,
    .readAttr                   = vNode_genericReadAttr,
    .writeAttr                  = vNode_genericWriteAttr,
    .addDirectoryEntry          = __devfs_vNode_addDirectoryEntry,
    .removeDirectoryEntry       = __devfs_vNode_removeDirectoryEntry,
    .renameDirectoryEntry       = __devfs_vNode_renameDirectoryEntry,
    .readDirectoryEntries       = __devfs_vNode_readDirectoryEntries
};

void devfsDirectoryEntry_initStruct(DevfsDirectoryEntry* entry, ConstCstring name, fsEntryType type, Index64 mappingIndex, FScore* fscore) {
    string_initStructStr(&entry->name, name);
    ERROR_GOTO_IF_ERROR(0);
    entry->mappingIndex = mappingIndex;
    entry->size         = type == FS_ENTRY_TYPE_DEVICE ? (Size)-1 : 0;
    entry->type         = type;

    return;
    ERROR_FINAL_BEGIN(0);
}

vNodeOperations* devfs_vNode_getOperations() {
    return &_devfs_vNodeOperations;
}

static void __devfs_vNode_readData(vNode* vnode, Index64 begin, void* buffer, Size byteN) {
    if (vnode->fsNode->type == FS_ENTRY_TYPE_FILE || vnode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY) {
        if (begin + byteN > vnode->sizeInByte) {
            ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
        }
        DevfsVnode* devfsVnode = HOST_POINTER(vnode, DevfsVnode, vnode);
        memory_memcpy(buffer, devfsVnode->data + begin, byteN);
        return;
    }

    Device* device = device_getDevice(vnode->deviceID);
    if (device == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    device_read(device, begin, buffer, byteN);
    ERROR_GOTO_IF_ERROR(0);
    
    return;
    ERROR_FINAL_BEGIN(0);
}

static void __devfs_vNode_writeData(vNode* vnode, Index64 begin, const void* buffer, Size byteN) {
    if (vnode->fsNode->type == FS_ENTRY_TYPE_FILE || vnode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY) {
        if (begin + byteN > vnode->sizeInByte) {
            ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
        }
        DevfsVnode* devfsVnode = HOST_POINTER(vnode, DevfsVnode, vnode);
        memory_memcpy(devfsVnode->data + begin, buffer, byteN);
        return;
    }
    
    Device* device = device_getDevice(vnode->deviceID);
    if (device == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    device_write(device, begin, buffer, byteN);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __devfs_vNode_resize(vNode* vnode, Size newSizeInByte) {
    if (!(vnode->fsNode->type == FS_ENTRY_TYPE_FILE || vnode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY)) {
        return;
    }

    void* newPages = NULL;
    
    DevfsVnode* devfsVnode = HOST_POINTER(vnode, DevfsVnode, vnode);
    Size newPageNum = DIVIDE_ROUND_UP(newSizeInByte, PAGE_SIZE), oldPageNum = DIVIDE_ROUND_UP(vnode->sizeInByte, PAGE_SIZE);
    
    if (newPageNum != oldPageNum) {
        if (newPageNum != 0) {
            newPages = mm_allocatePages(newPageNum);
            if (newPages == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }
            
            if (newPageNum < oldPageNum) {
                memory_memcpy(newPages, devfsVnode->data, newSizeInByte);
                memory_memset(newPages + newSizeInByte, 0, newPageNum * PAGE_SIZE - newSizeInByte);
            } else {
                memory_memcpy(newPages, devfsVnode->data, vnode->sizeInByte);
                memory_memset(newPages + vnode->sizeInByte, 0, newPageNum * PAGE_SIZE - vnode->sizeInByte);
            }
        }
        
        if (oldPageNum != 0) {
            mm_freePages(devfsVnode->data);
        }

        devfsVnode->data = newPages;
        devfscore_setStorageMapping(vnode->fsNode->physicalPosition, (Object)newPages);
    }

    DevfsNodeMetadata* metadata = devfscore_getMetadata(vnode, vnode->fsNode->physicalPosition);
    if (metadata == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    metadata->sizeInByte = vnode->sizeInByte = newSizeInByte;

    return;
    ERROR_FINAL_BEGIN(0);
    if (newPages != NULL) {
        mm_freePages(newPages);
    }
    return;
}

static Index64 __devfs_vNode_addDirectoryEntry(vNode* vnode, ConstCstring name, fsEntryType type, vNodeAttribute* attr, ID deviceID) {
    DEBUG_ASSERT_SILENT(vnode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);
    
    FScore* fscore = vnode->fscore;
    Devfscore* devfscore = HOST_POINTER(fscore, Devfscore, fscore);

    Size currentPointer = 0;
    DevfsDirectoryEntry devfsDirectoryEntry;
    while (currentPointer < vnode->sizeInByte) {
        vNode_rawReadData(vnode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
        ERROR_GOTO_IF_ERROR(0);
        
        if (cstring_strcmp(name, devfsDirectoryEntry.name.data) == 0 && type == devfsDirectoryEntry.type) {
            ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
            break;
        }

        currentPointer += sizeof(DevfsDirectoryEntry);
    }
    
    bool isRealData = (type == FS_ENTRY_TYPE_FILE || type == FS_ENTRY_TYPE_DIRECTORY);
    Object pointTo = isRealData ? (Object)NULL : (Object)deviceID;

    Index64 mappingIndex = devfscore_registerMetadata(devfscore, isRealData ? 0 : INFINITE, pointTo);
    ERROR_GOTO_IF_ERROR(0);

    devfsDirectoryEntry_initStruct(&devfsDirectoryEntry, name, type, mappingIndex, fscore);

    vNode_rawResize(vnode, vnode->sizeInByte + sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    vNode_rawWriteData(vnode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    return mappingIndex;
    ERROR_FINAL_BEGIN(0);

    return INVALID_INDEX64;
}

static void __devfs_vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory) {
    DEBUG_ASSERT_SILENT(vnode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    void* remainingData = NULL;
    
    FScore* fscore = vnode->fscore;
    Devfscore* devfscore = HOST_POINTER(fscore, Devfscore, fscore);

    Size currentPointer = 0;
    bool found = false;
    DevfsDirectoryEntry devfsDirectoryEntry;
    while (currentPointer < vnode->sizeInByte) {
        vNode_rawReadData(vnode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
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

    devfscore_unregisterMetadata(devfscore, devfsDirectoryEntry.mappingIndex);
    ERROR_GOTO_IF_ERROR(0);

    if (currentPointer + sizeof(DevfsDirectoryEntry) < vnode->sizeInByte) {
        Size remainingDataSize = vnode->sizeInByte - currentPointer - sizeof(DevfsDirectoryEntry);
        remainingData = mm_allocate(remainingDataSize);
        vNode_rawReadData(vnode, currentPointer, remainingData + sizeof(DevfsDirectoryEntry), remainingDataSize);
        ERROR_GOTO_IF_ERROR(0);
        vNode_rawWriteData(vnode, currentPointer, remainingData, remainingDataSize);
        ERROR_GOTO_IF_ERROR(0);
        mm_free(remainingData);
        remainingData = NULL;
    }

    vNode_rawResize(vnode, vnode->sizeInByte - sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
    if (remainingData != NULL) {
        mm_free(remainingData);
    }
}

static void __devfs_vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName) {
    DEBUG_ASSERT_SILENT(vnode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(moveTo->fsNode->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(vnode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    void* remainingData = NULL;

    //Remove from vnode directory
    
    Size currentPointer = 0;
    bool found = false;
    DevfsDirectoryEntry devfsDirectoryEntry, transplantDirEntry;
    while (currentPointer < vnode->sizeInByte) {
        vNode_rawReadData(vnode, currentPointer, &transplantDirEntry, sizeof(DevfsDirectoryEntry));
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

    if (currentPointer + sizeof(DevfsDirectoryEntry) < vnode->sizeInByte) {
        Size remainingDataSize = vnode->sizeInByte - currentPointer - sizeof(DevfsDirectoryEntry);
        remainingData = mm_allocate(remainingDataSize);
        vNode_rawReadData(vnode, currentPointer, remainingData + sizeof(DevfsDirectoryEntry), remainingDataSize);
        ERROR_GOTO_IF_ERROR(0);
        vNode_rawWriteData(vnode, currentPointer, remainingData, remainingDataSize);
        ERROR_GOTO_IF_ERROR(0);
        mm_free(remainingData);
        remainingData = NULL;
    }

    vNode_rawResize(vnode, vnode->sizeInByte - sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    //Write to moveTo directory

    currentPointer = 0;
    found = false;
    while (currentPointer < moveTo->sizeInByte) {
        vNode_rawReadData(moveTo, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
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
    
    vNode_rawResize(moveTo, moveTo->sizeInByte + sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    vNode_rawWriteData(moveTo, currentPointer, &transplantDirEntry, sizeof(DevfsDirectoryEntry));
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);

    if (remainingData != NULL) {
        mm_free(remainingData);
    }
}

static void __devfs_vNode_readDirectoryEntries(vNode* vnode) {
    DEBUG_ASSERT_SILENT(vnode->sizeInByte % sizeof(DevfsDirectoryEntry) == 0);

    void* blockBuffer = NULL;
    
    FScore* fscore = vnode->fscore;
    Devfscore* devfscore = HOST_POINTER(fscore, Devfscore, fscore);
    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* device = &targetBlockDevice->device;
    Size blockSize = POWER_2(device->granularity);
    DEBUG_ASSERT_SILENT(vnode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY);
    DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(vnode->fsNode);
    
    blockBuffer = mm_allocate(blockSize);
    if (blockBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    DEBUG_ASSERT_SILENT(dirNode->dirPart.childrenNum == FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM);
    dirNode->dirPart.childrenNum = 0;

    Size currentPointer = 0;
    DevfsDirectoryEntry devfsDirectoryEntry;
    DirectoryEntry directoryEntry;
    while (currentPointer < vnode->sizeInByte) {
        vNode_rawReadData(vnode, currentPointer, &devfsDirectoryEntry, sizeof(DevfsDirectoryEntry));
        ERROR_GOTO_IF_ERROR(0);

        directoryEntry.name         = devfsDirectoryEntry.name.data;
        directoryEntry.type         = devfsDirectoryEntry.type;
        directoryEntry.vnodeID      = 0;    //TODO: Re-implement vnode ID
        directoryEntry.mode         = DIRECTORY_ENTRY_MODE_ANY; //TODO: Support for mode
        if (fsEntryType_isDevice(directoryEntry.type)) {
            directoryEntry.size     = DIRECTORY_ENTRY_SIZE_ANY;
            directoryEntry.deviceID = (DeviceID)devfscore_getStorageMapping(devfsDirectoryEntry.mappingIndex);
        } else {
            directoryEntry.size     = devfsDirectoryEntry.size;
            directoryEntry.deviceID = DIRECTORY_ENTRY_DEVICE_ID_ANY;
        }

        fsnode_create(directoryEntry.name, directoryEntry.type, &dirNode->node, devfsDirectoryEntry.mappingIndex);
        ERROR_GOTO_IF_ERROR(0);
        
        currentPointer += sizeof(DevfsDirectoryEntry);
    }

    mm_free(blockBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (blockBuffer != NULL) {
        mm_free(blockBuffer);
    }
}