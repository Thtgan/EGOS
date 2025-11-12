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

static Index64 __fat32_vNode_addDirectoryEntry(vNode* vnode, DirectoryEntry* entry, FSnodeAttribute* attr);

static void __fat32_vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory);

static void __fat32_vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName);

static void __fat32_vNode_readDirectoryEntries(vNode* vnode);

static vNodeOperations _fat32_vNodeOperations = {
    .readData                   = __fat32_vNode_readData,
    .writeData                  = __fat32_vNode_writeData,
    .resize                     = __fat32_vNode_resize,
    .addDirectoryEntry          = __fat32_vNode_addDirectoryEntry,
    .removeDirectoryEntry       = __fat32_vNode_removeDirectoryEntry,
    .renameDirectoryEntry       = __fat32_vNode_renameDirectoryEntry,
    .readDirectoryEntries       = __fat32_vNode_readDirectoryEntries
};

static FAT32DirectoryEntry _fat32_vNode_directoryTail;

void fat32_vNode_init() {
    memory_memset(&_fat32_vNode_directoryTail, 0, sizeof(FAT32DirectoryEntry));
}

vNodeOperations* fat32_vNode_getOperations() {
    return &_fat32_vNodeOperations;
}

Size fat32_vNode_touchDirectory(vNode* vnode) {
    DirectoryEntry* entryNode = &vnode->fsNode->entry;
    DEBUG_ASSERT_SILENT(entryNode->type == FS_ENTRY_TYPE_DIRECTORY);
    
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

    Index64 currentPointer = 0;
    while (true) {
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd((FAT32UnknownTypeEntry*)entriesBuffer)) {
            break;
        }
        
        currentPointer += fat32_directoryEntry_getEntriesLength(entriesBuffer);
    }
    currentPointer += sizeof(FAT32DirectoryEntry);

    mm_free(clusterBuffer);
    mm_free(entriesBuffer);
    
    return currentPointer;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        mm_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        mm_free(entriesBuffer);
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

    byteN = algorithms_umin64(byteN, vnode->size - begin);    //TODO: Or fail when access exceeds limitation?
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

    byteN = algorithms_umin64(byteN, vnode->size - begin);    //TODO: Or fail when access exceeds limitation?
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
    DirectoryEntry* nodeEntry = &node->entry;
    FAT32vnode* fat32vnode = HOST_POINTER(vnode, FAT32vnode, vnode);

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    DEBUG_ASSERT_SILENT(nodeEntry->type == FS_ENTRY_TYPE_FILE || nodeEntry->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(device_isBlockDevice(targetDevice));

    FAT32BPB* BPB = fat32fscore->BPB;
    Size clusterSize = POWER_2(targetDevice->granularity) * BPB->sectorPerCluster;

    Index32 currentClusterIndex = fat32_getCluster(fat32fscore, fat32vnode->firstCluster, begin / clusterSize);
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
    DirectoryEntry* nodeEntry = &node->entry;
    FAT32vnode* fat32vnode = HOST_POINTER(vnode, FAT32vnode, vnode);

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    DEBUG_ASSERT_SILENT(nodeEntry->type == FS_ENTRY_TYPE_FILE || nodeEntry->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(device_isBlockDevice(targetDevice));

    FAT32BPB* BPB = fat32fscore->BPB;
    Size clusterSize = POWER_2(targetDevice->granularity) * BPB->sectorPerCluster;

    Index32 currentClusterIndex = fat32_getCluster(fat32fscore, fat32vnode->firstCluster, begin / clusterSize);
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
    DirectoryEntry* nodeEntry = &vnode->fsNode->entry;
    if (nodeEntry->type == FS_ENTRY_TYPE_DIRECTORY) {
        DEBUG_ASSERT_SILENT(vnode->size == 0);
    }

    Index32 freeClusterChain = INVALID_INDEX32;
    FAT32fscore* fat32fscore = HOST_POINTER(vnode->fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;
    Device* fscoreDevice = &vnode->fscore->blockDevice->device;
    FAT32vnode* fat32vnode = HOST_POINTER(vnode, FAT32vnode, vnode);

    Size newSizeInCluster = DIVIDE_ROUND_UP(DIVIDE_ROUND_UP_SHIFT(newSizeInByte, fscoreDevice->granularity), BPB->sectorPerCluster), oldSizeInCluster = DIVIDE_ROUND_UP(DIVIDE_ROUND_UP_SHIFT(vnode->tokenSpaceSize, fscoreDevice->granularity), BPB->sectorPerCluster);
    if (newSizeInByte == 0) {
        newSizeInCluster = 1;
    }
    
    if (newSizeInCluster < oldSizeInCluster) {
        Index32 tail = fat32_getCluster(fat32fscore, fat32vnode->firstCluster, newSizeInCluster - 1);
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

        Index32 tail = fat32_getCluster(fat32fscore, fat32vnode->firstCluster, oldSizeInCluster - 1);
        if (freeClusterChain == INVALID_INDEX32) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        fat32_insertClusterChain(fat32fscore, tail, freeClusterChain);
    }

    vnode->tokenSpaceSize = newSizeInCluster * BPB->sectorPerCluster * POWER_2(fscoreDevice->granularity);
    nodeEntry->size = vnode->size = newSizeInByte;

    return;
    ERROR_FINAL_BEGIN(0);
    if (freeClusterChain != INVALID_INDEX32) {
        fat32_freeClusterChain(fat32fscore, freeClusterChain);
    }
}

static Index64 __fat32_vNode_addDirectoryEntry(vNode* vnode, DirectoryEntry* entry, FSnodeAttribute* attr) {
    DEBUG_ASSERT_SILENT(directoryEntry_checkAdding(entry));
    if (!(entry->type == FS_ENTRY_TYPE_DIRECTORY || entry->type == FS_ENTRY_TYPE_FILE)) {
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
    FSnodeAttribute fsnodeAttribute;
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

        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &fsnodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(entry->name, entryName.data) == 0 && (entry->type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {            
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

    entry->pointsTo = (Index64)newFirstCluster;
    entry->size = 0;

    entriesLength = fat32_directoryEntry_initEntries(entriesBuffer, entry->name, entry->type, attr, newFirstCluster, 0);
    ERROR_GOTO_IF_ERROR(0);

    vNode_rawResize(vnode, vnode->size + entriesLength);
    ERROR_GOTO_IF_ERROR(0);

    __fat32_vNode_doWriteData(vnode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    currentPointer += entriesLength;

    __fat32_vNode_doWriteData(vnode, currentPointer, &_fat32_vNode_directoryTail, sizeof(FAT32DirectoryEntry), clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);

    mm_free(clusterBuffer);
    mm_free(entriesBuffer);

    return (Index64)newFirstCluster;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        mm_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        mm_free(entriesBuffer);
    }

    return INVALID_INDEX64;
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
    FSnodeAttribute fsnodeAttribute;
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

        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &fsnodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(name, entryName.data) == 0 && isDirectory == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += entriesLength;
    }

    if (!found) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    DEBUG_ASSERT_SILENT(entriesLength > 0);
    DEBUG_ASSERT_SILENT(currentPointer + entriesLength < vnode->size);
    Size remainingDataSize = vnode->size - currentPointer - entriesLength;
    remainingData = mm_allocate(remainingDataSize);
    __fat32_vNode_doReadData(vnode, currentPointer + entriesLength, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    __fat32_vNode_doWriteData(vnode, currentPointer, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    mm_free(remainingData);
    remainingData = NULL;
    
    vNode_rawResize(vnode, vnode->size - entriesLength);
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
    FSnodeAttribute fsnodeAttribute;
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

        fat32_directoryEntry_parse(transplantEntriesBuffer, &entryName, &attribute, &fsnodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(entry->entry.name, entryName.data) == 0 && (entry->entry.type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += entriesLength;
    }

    if (!found) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    DEBUG_ASSERT_SILENT(entriesLength > 0);
    DEBUG_ASSERT_SILENT(currentPointer + entriesLength < vnode->size);
    Size remainingDataSize = vnode->size - currentPointer - entriesLength;
    remainingData = mm_allocate(remainingDataSize);
    __fat32_vNode_doReadData(vnode, currentPointer + entriesLength, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    __fat32_vNode_doWriteData(vnode, currentPointer, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    mm_free(remainingData);
    remainingData = NULL;
    
    vNode_rawResize(vnode, vnode->size - entriesLength);
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

        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &fsnodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(entry->entry.name, entryName.data) == 0 && (entry->entry.type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += entriesLength;
    }

    if (found) {
        ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
    }

    DEBUG_ASSERT_SILENT(entriesLength > 0);
    vNode_rawResize(moveTo, moveTo->size + entriesLength);
    ERROR_GOTO_IF_ERROR(0);

    __fat32_vNode_doWriteData(moveTo, currentPointer, transplantEntriesBuffer, entriesLength, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    currentPointer += entriesLength;

    __fat32_vNode_doWriteData(vnode, currentPointer, &_fat32_vNode_directoryTail, sizeof(FAT32DirectoryEntry), clusterBuffer);
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

static void __fat32_vNode_readDirectoryEntries(vNode* vnode) {
    void* clusterBuffer = NULL, * entriesBuffer = NULL;
    
    FScore* fscore = vnode->fscore;
    FAT32fscore* fat32fscore = HOST_POINTER(fscore, FAT32fscore, fscore);
    FAT32BPB* BPB = fat32fscore->BPB;
    DirectoryEntry* nodeEntry = &vnode->fsNode->entry;
    DEBUG_ASSERT_SILENT(nodeEntry->type == FS_ENTRY_TYPE_DIRECTORY);
    DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(vnode->fsNode);

    BlockDevice* targetBlockDevice = fscore->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    Size clusterSize = POWER_2(targetDevice->granularity) * BPB->sectorPerCluster;
    clusterBuffer = mm_allocate(clusterSize);
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
    FSnodeAttribute fsnodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    Size entriesLength = 0;

    FAT32vnode* fat32vnode = HOST_POINTER(vnode, FAT32vnode, vnode);
    Index32 currentClusterIndex = fat32_getCluster(fat32fscore, fat32vnode->firstCluster, 0);

    DEBUG_ASSERT_SILENT(dirNode->dirPart.childrenNum == FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM);
    dirNode->dirPart.childrenNum = 0;
    while (true) {
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }
        
        entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_vNode_doReadData(vnode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        
        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &fsnodeAttribute, &firstCluster, &size);
        fsEntryType type = TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY) ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE;

        DEBUG_ASSERT_SILENT(type != FS_ENTRY_TYPE_DIRECTORY || size == 0);

        DirectoryEntry newDirEntry = (DirectoryEntry) {
            .name = entryName.data,
            .type = type,
            .mode = 0,
            .vnodeID = currentClusterIndex * clusterSize | (currentPointer % clusterSize),
            .size = size,
            .pointsTo = (Index64)firstCluster
        };

        fsnode_create(&newDirEntry, INFINITE, &fsnodeAttribute, &dirNode->node);

        Size stepClusterNum = DIVIDE_ROUND_DOWN(currentPointer + entriesLength, clusterSize) - DIVIDE_ROUND_DOWN(currentPointer, clusterSize);
        currentPointer += entriesLength;
        if (stepClusterNum != 0) {
            currentClusterIndex = fat32_getCluster(fat32fscore, currentClusterIndex, stepClusterNum);
        }
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