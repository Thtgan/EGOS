#include<fs/fat32/vnode.h>

#include<devices/charDevice.h>
#include<fs/directoryEntry.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/directoryEntry.h>
#include<fs/fat32/fat32.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/vnode.h>
#include<fs/fscore.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>
#include<algorithms.h>
#include<cstring.h>

static void __fat32_vNode_readData(vNode* vnode, Index64 begin, void* buffer, Size byteN);

static void __fat32_vNode_writeData(vNode* vnode, Index64 begin, const void* buffer, Size byteN);

static void __fat32_vNode_doReadData(vNode* vnode, Index64 begin, void* buffer, Size byteN, void* clusterBuffer);

static void __fat32_vNode_doWriteData(vNode* vnode, Index64 begin, const void* buffer, Size byteN, void* clusterBuffer);

static void __fat32_vNode_resize(vNode* vnode, Size newSizeInByte);

static void __fat32_vNode_iterateDirectoryEntries(vNode* vnode, vNodeOperationIterateDirectoryEntryFunc func, Object arg, void* ret);

static void __fat32_vNode_addDirectoryEntry(vNode* vnode, ConstCstring name, fsEntryType type, vNodeAttribute* attr, ID deviceID);

static void __fat32_vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory);

static void __fat32_vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName);

static vNodeOperations _fat32_vNodeOperations = {
    .readData                   = __fat32_vNode_readData,
    .writeData                  = __fat32_vNode_writeData,
    .resize                     = __fat32_vNode_resize,
    .readAttr                   = vNode_genericReadAttr,
    .writeAttr                  = vNode_genericWriteAttr,
    .iterateDirectoryEntries    = __fat32_vNode_iterateDirectoryEntries,
    .addDirectoryEntry          = __fat32_vNode_addDirectoryEntry,
    .removeDirectoryEntry       = __fat32_vNode_removeDirectoryEntry,
    .renameDirectoryEntry       = __fat32_vNode_renameDirectoryEntry
};

static FAT32DirectoryEntry _fat32_vNode_directoryTail;

void fat32_vNode_init() {
    memory_memset(&_fat32_vNode_directoryTail, 0, sizeof(FAT32DirectoryEntry));
}

vNodeOperations* fat32_vNode_getOperations() {
    return &_fat32_vNodeOperations;
}

Size fat32_vNode_touchDirectory(vNode* vnode) {
    DEBUG_ASSERT_SILENT(vnode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY && !HOST_POINTER(vnode, FAT32Vnode, vnode)->isTouched);
    
    void* clusterBuffer = NULL, * entriesBuffer = NULL;
    
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = mm_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = mm_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    String entryName;
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    vNodeAttribute vnodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    while (true) {
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd((FAT32UnknownTypeEntry*)entriesBuffer)) {
            break;
        }

        Size entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        
        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &vnodeAttribute, &firstCluster, &size);

        fsEntryType entryType = TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY) ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE;
        ID entryVnodeID = fscore_allocateVnodeID(fscore);
        DirectoryEntry entry = (DirectoryEntry) {
            .name = entryName.data,
            .type = entryType,
            .vnodeID = entryVnodeID,
            .size = size,
            .mode = DIRECTORY_ENTRY_MODE_ANY,
            .deviceID = DIRECTORY_ENTRY_DEVICE_ID_ANY
        };

        fat32FScore_registerMetadata(fat32fscore, &entry, vnode->fsNode, firstCluster, &vnodeAttribute);
        ERROR_GOTO_IF_ERROR(0);
        
        currentPointer += entriesLength;
    }

    mm_free(clusterBuffer);
    mm_free(entriesBuffer);

    string_clearStruct(&entryName);
    
    return currentPointer + sizeof(FAT32DirectoryEntry);
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        mm_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        mm_free(entriesBuffer);
    }

    if (string_isAvailable(&entryName)) {
        string_clearStruct(&entryName);
    }

    return 0;
}

static void __fat32_vNode_readData(vNode* vnode, Index64 begin, void* buffer, Size byteN) {
    void* clusterBuffer = NULL;
    
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = mm_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    byteN = algorithms_umin64(byteN, vnode->sizeInByte - begin);    //TODO: Or fail when access exceeds limitation?
    __fat32_vNode_doReadData(vnode, begin, buffer, byteN, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);

    mm_free(clusterBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        mm_free(clusterBuffer);
    }
}

static void __fat32_vNode_writeData(vNode* vnode, Index64 begin, const void* buffer, Size byteN) {
    void* clusterBuffer = NULL;
    
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = mm_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    byteN = algorithms_umin64(byteN, vnode->sizeInByte - begin);    //TODO: Or fail when access exceeds limitation?
    __fat32_vNode_doWriteData(vnode, begin, buffer, byteN, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);

    mm_free(clusterBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        mm_free(clusterBuffer);
    }
}

static void __fat32_vNode_doReadData(vNode* vnode, Index64 begin, void* buffer, Size byteN, void* clusterBuffer) {
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    
    fsNode* node = vnode->fsNode;
    FAT32Vnode* fat32Vnode = HOST_POINTER(vnode, FAT32Vnode, vnode);

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    DEBUG_ASSERT_SILENT(node->type == FS_ENTRY_TYPE_FILE || node->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(device_isBlockDevice(targetDevice));

    FAT32BPB* BPB = fat32fscore->BPB;
    Size clusterSize = POWER_2(targetDevice->granularity) * BPB->sectorPerCluster;

    Index32 currentClusterIndex = fat32_getCluster(fat32fscore, fat32Vnode->firstCluster, begin / clusterSize);
    if (currentClusterIndex == INVALID_INDEX32) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size remainByteNum = byteN;
    if (begin % clusterSize != 0) {
        Index64 offsetInCluster = begin % clusterSize;
        blockDevice_readBlocks(targetBlockDevice, (Index64)currentClusterIndex * BPB->sectorPerCluster + fat32fscore->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);

        Size byteReadN = algorithms_umin64(remainByteNum, clusterSize - offsetInCluster);
        memory_memcpy(buffer, clusterBuffer + offsetInCluster, byteReadN);
        
        currentClusterIndex = fat32_getCluster(fat32fscore, currentClusterIndex, 1);

        buffer += byteReadN;
        remainByteNum -= byteReadN;
    }

    if (remainByteNum >= clusterSize) {
        Size remainingFullClusterNum = remainByteNum / clusterSize;
        while (remainingFullClusterNum > 0) {
            DEBUG_ASSERT_SILENT(currentClusterIndex != INVALID_INDEX32);

            Size continousClusterLength = 0;
            Index32 nextClusterIndex = fat32_stepCluster(fat32fscore, currentClusterIndex, remainingFullClusterNum, &continousClusterLength);
            DEBUG_ASSERT_SILENT(VALUE_WITHIN(0, remainingFullClusterNum, continousClusterLength, <, <=));

            Size continousBlockLength = continousClusterLength * BPB->sectorPerCluster;
            blockDevice_readBlocks(targetBlockDevice, (Index64)currentClusterIndex * BPB->sectorPerCluster + fat32fscore->dataBlockRange.begin, buffer, continousBlockLength);
            ERROR_GOTO_IF_ERROR(0);

            currentClusterIndex = nextClusterIndex;

            buffer += continousClusterLength * clusterSize;
            remainingFullClusterNum -= continousClusterLength;
        }

        remainByteNum %= clusterSize;
    }

    if (remainByteNum > 0) {
        DEBUG_ASSERT_SILENT(currentClusterIndex != INVALID_INDEX32);
        blockDevice_readBlocks(targetBlockDevice, (Index64)currentClusterIndex * BPB->sectorPerCluster + fat32fscore->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);

        memory_memcpy(buffer, clusterBuffer, remainByteNum);

        remainByteNum = 0;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __fat32_vNode_doWriteData(vNode* vnode, Index64 begin, const void* buffer, Size byteN, void* clusterBuffer) {
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    
    fsNode* node = vnode->fsNode;
    FAT32Vnode* fat32Vnode = HOST_POINTER(vnode, FAT32Vnode, vnode);

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    DEBUG_ASSERT_SILENT(node->type == FS_ENTRY_TYPE_FILE || node->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(device_isBlockDevice(targetDevice));

    FAT32BPB* BPB = fat32fscore->BPB;
    Size clusterSize = POWER_2(targetDevice->granularity) * BPB->sectorPerCluster;

    Index32 currentClusterIndex = fat32_getCluster(fat32fscore, fat32Vnode->firstCluster, begin / clusterSize);
    if (currentClusterIndex == INVALID_INDEX32) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size remainByteNum = byteN;
    if (begin % clusterSize != 0) {
        Index64 offsetInCluster = begin % clusterSize;
        blockDevice_readBlocks(targetBlockDevice, (Index64)currentClusterIndex * BPB->sectorPerCluster + fat32fscore->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);
        
        Size byteReadN = algorithms_umin64(remainByteNum, clusterSize - offsetInCluster);
        memory_memcpy(clusterBuffer + offsetInCluster, buffer, byteReadN);
        
        blockDevice_writeBlocks(targetBlockDevice, (Index64)currentClusterIndex * BPB->sectorPerCluster + fat32fscore->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);

        currentClusterIndex = fat32_getCluster(fat32fscore, currentClusterIndex, 1);

        buffer += byteReadN;
        remainByteNum -= byteReadN;
    }

    if (remainByteNum >= clusterSize) {
        Size remainingFullClusterNum = remainByteNum / clusterSize;
        while (remainingFullClusterNum > 0) {
            DEBUG_ASSERT_SILENT(currentClusterIndex != INVALID_INDEX32);

            Size continousClusterLength = 0;
            Index32 nextClusterIndex = fat32_stepCluster(fat32fscore, currentClusterIndex, remainingFullClusterNum, &continousClusterLength);
            DEBUG_ASSERT_SILENT(VALUE_WITHIN(0, remainingFullClusterNum, continousClusterLength, <, <=));

            Size continousBlockLength = continousClusterLength * BPB->sectorPerCluster;
            blockDevice_writeBlocks(targetBlockDevice, (Index64)currentClusterIndex * BPB->sectorPerCluster + fat32fscore->dataBlockRange.begin, buffer, continousBlockLength);
            ERROR_GOTO_IF_ERROR(0);

            currentClusterIndex = nextClusterIndex;

            buffer += continousClusterLength * clusterSize;
            remainingFullClusterNum -= continousClusterLength;
        }

        remainByteNum %= clusterSize;
    }

    if (remainByteNum > 0) {
        DEBUG_ASSERT_SILENT(currentClusterIndex != INVALID_INDEX32);
        blockDevice_readBlocks(targetBlockDevice, (Index64)currentClusterIndex * BPB->sectorPerCluster + fat32fscore->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);
        
        memory_memcpy(clusterBuffer, buffer, remainByteNum);

        blockDevice_writeBlocks(targetBlockDevice, (Index64)currentClusterIndex * BPB->sectorPerCluster + fat32fscore->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);
        
        remainByteNum = 0;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __fat32_vNode_resize(vNode* vnode, Size newSizeInByte) {
    fsNode* fsNode = vnode->fsNode;
    if (fsNode->type == FS_ENTRY_TYPE_DIRECTORY) {
        DEBUG_ASSERT_SILENT(vnode->sizeInByte == 0);
    }

    Index32 freeClusterChain = INVALID_INDEX32;
    FAT32fscore* fat32fscore = HOST_POINTER(vnode->fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;
    Device* fscoreDevice = &vnode->fscore->blockDevice->device;
    FAT32Vnode* fat32Vnode = HOST_POINTER(vnode, FAT32Vnode, vnode);

    Size newSizeInCluster = DIVIDE_ROUND_UP(DIVIDE_ROUND_UP_SHIFT(newSizeInByte, fscoreDevice->granularity), BPB->sectorPerCluster), oldSizeInCluster = DIVIDE_ROUND_UP(vnode->sizeInBlock, BPB->sectorPerCluster);
    if (newSizeInByte == 0) {
        newSizeInCluster = 1;
    }
    
    if (newSizeInCluster < oldSizeInCluster) {
        Index32 tail = fat32_getCluster(fat32fscore, fat32Vnode->firstCluster, newSizeInCluster - 1);
        if (tail == INVALID_INDEX32) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        Index32 cut = fat32_cutClusterChain(fat32fscore, tail);
        fat32_freeClusterChain(fat32fscore, cut);
    } else if (newSizeInCluster > oldSizeInCluster) {
        freeClusterChain = fat32_allocateClusterChain(fat32fscore, newSizeInCluster - oldSizeInCluster);
        if (freeClusterChain == INVALID_INDEX32) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        Index32 tail = fat32_getCluster(fat32fscore, fat32Vnode->firstCluster, oldSizeInCluster - 1);
        if (freeClusterChain == INVALID_INDEX32) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        fat32_insertClusterChain(fat32fscore, tail, freeClusterChain);
    }

    vnode->sizeInBlock = newSizeInCluster * BPB->sectorPerCluster;
    vnode->sizeInByte = newSizeInByte;

    return;
    ERROR_FINAL_BEGIN(0);
    if (freeClusterChain != INVALID_INDEX32) {
        fat32_freeClusterChain(fat32fscore, freeClusterChain);
    }
}

static void __fat32_vNode_iterateDirectoryEntries(vNode* vnode, vNodeOperationIterateDirectoryEntryFunc func, Object arg, void* ret) {
    void* clusterBuffer = NULL, * entriesBuffer = NULL;
    
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = mm_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = mm_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    String entryName;
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    vNodeAttribute vnodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    Size entriesLength = 0;
    DirectoryEntry directoryEntry;
    while (true) {
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }
        
        entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        
        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &vnodeAttribute, &firstCluster, &size);
        fsEntryType type = TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY) ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE;

        FAT32NodeMetadata* metadata = fat32FScore_getMetadataFromFirstCluster(fat32fscore, firstCluster);
        if (metadata == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_CLEAR();
        }

        directoryEntry.name = entryName.data;
        directoryEntry.type = type;
        directoryEntry.vnodeID = FAT32_NODE_METADATA_GET_VNODE_ID(metadata);
        directoryEntry.size = size;
        directoryEntry.mode = DIRECTORY_ENTRY_MODE_ANY; //TODO: Support for mode
        directoryEntry.deviceID = DIRECTORY_ENTRY_DEVICE_ID_ANY;

        if (func(vnode, &directoryEntry, arg, ret)) {
            break;
        }
        ERROR_GOTO_IF_ERROR(0);

        currentPointer += entriesLength;
    }

    mm_free(clusterBuffer);
    mm_free(entriesBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        mm_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        mm_free(entriesBuffer);
    }
    return;
}

static void __fat32_vNode_addDirectoryEntry(vNode* vnode, ConstCstring name, fsEntryType type, vNodeAttribute* attr, ID deviceID) {
    if (!(type == FS_ENTRY_TYPE_DIRECTORY || type == FS_ENTRY_TYPE_FILE)) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }
    void* clusterBuffer = NULL, * entriesBuffer = NULL;
    
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = mm_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = mm_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    bool found = false;
    String entryName;
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    vNodeAttribute vnodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    Size entriesLength = 0;
    while (!found) {
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }

        entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);

        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &vnodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(name, entryName.data) == 0 && (type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {            
            found = true;
            break;
        }
        
        currentPointer += entriesLength;
    }

    if (found) {
        ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
    }

    Index32 newFirstCluster = fat32FScore_createFirstCluster(fat32fscore);
    if (newFirstCluster == INVALID_INDEX32) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    //TODO: Add '.' and '..' for directory

    entriesLength = fat32_directoryEntry_initEntries(entriesBuffer, name, type, attr, newFirstCluster, 0);
    ERROR_GOTO_IF_ERROR(0);

    vNode_rawResize(vnode, vnode->sizeInByte + entriesLength);
    ERROR_GOTO_IF_ERROR(0);

    __fat32_vNode_doWriteData(vnode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    currentPointer += entriesLength;

    __fat32_vNode_doWriteData(vnode, currentPointer, &_fat32_vNode_directoryTail, sizeof(FAT32DirectoryEntry), clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);

    mm_free(clusterBuffer);
    mm_free(entriesBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        mm_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        mm_free(entriesBuffer);
    }
}

static void __fat32_vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory) {
    void* clusterBuffer = NULL, * entriesBuffer = NULL, * remainingData = NULL;
    
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = mm_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = mm_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    bool found = false;
    String entryName;
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    vNodeAttribute vnodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    Size entriesLength = 0;
    while (!found) {
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }

        entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);

        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &vnodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(name, entryName.data) == 0 && isDirectory == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += entriesLength;
    }

    if (!found) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    fat32FScore_unregisterMetadata(fat32fscore, firstCluster);
    ERROR_GOTO_IF_ERROR(0);

    //TODO: unregister metadata here

    DEBUG_ASSERT_SILENT(entriesLength > 0);
    DEBUG_ASSERT_SILENT(currentPointer + entriesLength < vnode->sizeInByte);
    Size remainingDataSize = vnode->sizeInByte - currentPointer - entriesLength;
    remainingData = mm_allocate(remainingDataSize);
    __fat32_vNode_doReadData(vnode, currentPointer + entriesLength, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    __fat32_vNode_doWriteData(vnode, currentPointer, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    mm_free(remainingData);
    remainingData = NULL;
    
    vNode_rawResize(vnode, vnode->sizeInByte - entriesLength);
    ERROR_GOTO_IF_ERROR(0);
    
    mm_free(clusterBuffer);
    mm_free(entriesBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        mm_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        mm_free(entriesBuffer);
    }

    if (remainingData != NULL) {
        mm_free(remainingData);
    }
}

static void __fat32_vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName) {
    void* clusterBuffer = NULL, *entriesBuffer = NULL, * transplantEntriesBuffer = NULL, * remainingData = NULL;
    
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = mm_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = mm_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    transplantEntriesBuffer = mm_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (transplantEntriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    //Remove from vnode directory

    bool found = false;
    String entryName;
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    vNodeAttribute vnodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    Size entriesLength = 0;
    while (!found) {
        __fat32_vNode_doReadData(vnode, currentPointer, transplantEntriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }

        entriesLength = fat32_directoryEntry_getEntriesLength(transplantEntriesBuffer);
        __fat32_vNode_doReadData(vnode, currentPointer, transplantEntriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);

        fat32_directoryEntry_parse(transplantEntriesBuffer, &entryName, &attribute, &vnodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(entry->name.data, entryName.data) == 0 && (entry->type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += entriesLength;
    }

    if (!found) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    DEBUG_ASSERT_SILENT(entriesLength > 0);
    DEBUG_ASSERT_SILENT(currentPointer + entriesLength < vnode->sizeInByte);
    Size remainingDataSize = vnode->sizeInByte - currentPointer - entriesLength;
    remainingData = mm_allocate(remainingDataSize);
    __fat32_vNode_doReadData(vnode, currentPointer + entriesLength, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    __fat32_vNode_doWriteData(vnode, currentPointer, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    mm_free(remainingData);
    remainingData = NULL;
    
    vNode_rawResize(vnode, vnode->sizeInByte - entriesLength);
    ERROR_GOTO_IF_ERROR(0);

    //Write to moveTo directory

    currentPointer = 0;
    Size transplantEntriesLength = entriesLength;
    entriesLength = 0;
    found = false;
    while (!found) {
        __fat32_vNode_doReadData(moveTo, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }

        entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_vNode_doReadData(moveTo, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);

        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &vnodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(entry->name.data, entryName.data) == 0 && (entry->type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += entriesLength;
    }

    if (found) {
        ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
    }

    DEBUG_ASSERT_SILENT(entriesLength > 0);
    vNode_rawResize(moveTo, moveTo->sizeInByte + entriesLength);
    ERROR_GOTO_IF_ERROR(0);

    __fat32_vNode_doWriteData(moveTo, currentPointer, transplantEntriesBuffer, entriesLength, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    currentPointer += entriesLength;

    __fat32_vNode_doWriteData(vnode, currentPointer, &_fat32_vNode_directoryTail, sizeof(FAT32DirectoryEntry), clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);

    fsNode_transplant(entry, moveTo->fsNode);
    ERROR_GOTO_IF_ERROR(0);

    mm_free(clusterBuffer);
    mm_free(entriesBuffer);
    mm_free(transplantEntriesBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        mm_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        mm_free(entriesBuffer);
    }

    if (transplantEntriesBuffer != NULL) {
        mm_free(transplantEntriesBuffer);
    }

    if (remainingData != NULL) {
        mm_free(remainingData);
    }
}