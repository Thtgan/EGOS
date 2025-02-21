#include<fs/fat32/inode.h>

#include<devices/char/charDevice.h>
#include<fs/directoryEntry.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/directoryEntry.h>
#include<fs/fat32/fat32.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>
#include<algorithms.h>
#include<cstring.h>

static void __fat32_iNode_readData(iNode* inode, Index64 begin, void* buffer, Size byteN);

static void __fat32_iNode_writeData(iNode* inode, Index64 begin, const void* buffer, Size byteN);

static void __fat32_iNode_doReadData(iNode* inode, Index64 begin, void* buffer, Size byteN, void* clusterBuffer);

static void __fat32_iNode_doWriteData(iNode* inode, Index64 begin, const void* buffer, Size byteN, void* clusterBuffer);

static void __fat32_iNode_resize(iNode* inode, Size newSizeInByte);

static void __fat32_iNode_iterateDirectoryEntries(iNode* inode, iNodeOperationIterateDirectoryEntryFunc func, Object arg, void* ret);

static void __fat32_iNode_addDirectoryEntry(iNode* inode, ConstCstring name, fsEntryType type, iNodeAttribute* attr, ID deviceID);

static void __fat32_iNode_removeDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory);

static void __fat32_iNode_renameDirectoryEntry(iNode* inode, fsNode* entry, iNode* moveTo, ConstCstring newName);

static iNodeOperations _fat32_iNodeOperations = {
    .readData               = __fat32_iNode_readData,
    .writeData              = __fat32_iNode_writeData,
    .resize                 = __fat32_iNode_resize,
    .readAttr               = iNode_genericReadAttr,
    .writeAttr              = iNode_genericWriteAttr,
    .addDirectoryEntry      = __fat32_iNode_addDirectoryEntry,
    .removeDirectoryEntry   = __fat32_iNode_removeDirectoryEntry,
    .renameDirectoryEntry   = __fat32_iNode_renameDirectoryEntry
};

typedef struct {
    Index64 firstCluster;
} __FAT32iNodeInfo;

static FAT32DirectoryEntry _fat32_iNode_directoryTail;

void fat32_iNode_init() {
    memory_memset(&_fat32_iNode_directoryTail, 0, sizeof(FAT32DirectoryEntry));
}

iNodeOperations* fat32_iNode_getOperations() {
    return &_fat32_iNodeOperations;
}

Size fat32_iNode_touchDirectory(iNode* inode) {
    DEBUG_ASSERT_SILENT(inode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY && !HOST_POINTER(inode, FAT32Inode, inode)->isTouched);
    
    void* clusterBuffer = NULL, * entriesBuffer = NULL;
    fsNode* entryFSnode = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32BPB* BPB = fat32SuperBlock->BPB;

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = memory_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = memory_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    String entryName;   //TODO: Not necessary
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    iNodeAttribute inodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    while (true) {
        __fat32_iNode_doReadData(inode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd((FAT32UnknownTypeEntry*)entriesBuffer)) {
            break;
        }

        Size entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_iNode_doReadData(inode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        
        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &inodeAttribute, &firstCluster, &size);

        fsEntryType type = TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY) ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE;
        entryFSnode = fsNode_create(entryName.data, type, inode->fsNode);
        if (entryFSnode == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        ID entryInodeID = superBlock_allocateInodeID(superBlock);
        entryFSnode->inodeID = entryInodeID;    //TODO: Ugly code

        fat32SuperBlock_registerMetadata(fat32SuperBlock, firstCluster, entryInodeID, entryFSnode, &inodeAttribute, size);
        ERROR_GOTO_IF_ERROR(0);
        entryFSnode = NULL;
        
        currentPointer += entriesLength;
    }

    memory_free(clusterBuffer);
    memory_free(entriesBuffer);

    string_clearStruct(&entryName);
    
    return currentPointer + sizeof(FAT32DirectoryEntry);
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        memory_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        memory_free(entriesBuffer);
    }

    if (entryFSnode != NULL) {
        fsNode_release(entryFSnode);
    }

    if (string_isAvailable(&entryName)) {
        string_clearStruct(&entryName);
    }

    return 0;
}

static void __fat32_iNode_readData(iNode* inode, Index64 begin, void* buffer, Size byteN) {
    void* clusterBuffer = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32BPB* BPB = fat32SuperBlock->BPB;

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = memory_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    byteN = algorithms_umin64(byteN, inode->sizeInByte - begin);    //TODO: Or fail when access exceeds limitation?
    __fat32_iNode_doReadData(inode, begin, buffer, byteN, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);

    memory_free(clusterBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        memory_free(clusterBuffer);
    }
}

static void __fat32_iNode_writeData(iNode* inode, Index64 begin, const void* buffer, Size byteN) {
    void* clusterBuffer = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32BPB* BPB = fat32SuperBlock->BPB;

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = memory_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    byteN = algorithms_umin64(byteN, inode->sizeInByte - begin);    //TODO: Or fail when access exceeds limitation?
    __fat32_iNode_doWriteData(inode, begin, buffer, byteN, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);

    memory_free(clusterBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        memory_free(clusterBuffer);
    }
}

static void __fat32_iNode_doReadData(iNode* inode, Index64 begin, void* buffer, Size byteN, void* clusterBuffer) {
    SuperBlock* superBlock = inode->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    
    fsNode* node = inode->fsNode;
    FAT32Inode* fat32Inode = HOST_POINTER(inode, FAT32Inode, inode);

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    DEBUG_ASSERT_SILENT(node->type == FS_ENTRY_TYPE_FILE || node->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(device_isBlockDevice(targetDevice));

    FAT32BPB* BPB = fat32SuperBlock->BPB;
    Size clusterSize = POWER_2(targetDevice->granularity) * BPB->sectorPerCluster;

    Index64 currentClusterIndex = fat32_getCluster(fat32SuperBlock, fat32Inode->firstCluster, begin / clusterSize);
    if (currentClusterIndex == (Uint32)INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size remainByteNum = byteN;
    if (begin % clusterSize != 0) {
        Index64 offsetInCluster = begin % clusterSize;
        blockDevice_readBlocks(targetBlockDevice, currentClusterIndex * BPB->sectorPerCluster + fat32SuperBlock->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);

        Size byteReadN = algorithms_umin64(remainByteNum, clusterSize - offsetInCluster);
        memory_memcpy(buffer, clusterBuffer + offsetInCluster, byteReadN);
        
        currentClusterIndex = fat32_getCluster(fat32SuperBlock, currentClusterIndex, 1);

        buffer += byteReadN;
        remainByteNum -= byteReadN;
    }

    if (remainByteNum >= clusterSize) {
        Size remainingFullClusterNum = remainByteNum / clusterSize;
        while (remainingFullClusterNum > 0) {
            DEBUG_ASSERT_SILENT(currentClusterIndex != (Uint32)INVALID_INDEX);

            Size continousClusterLength = 0;
            Index32 nextClusterIndex = fat32_stepCluster(fat32SuperBlock, currentClusterIndex, remainingFullClusterNum, &continousClusterLength);
            DEBUG_ASSERT_SILENT(VALUE_WITHIN(0, remainingFullClusterNum, continousClusterLength, <, <=));

            Size continousBlockLength = continousClusterLength * BPB->sectorPerCluster;
            blockDevice_readBlocks(targetBlockDevice, currentClusterIndex * BPB->sectorPerCluster + fat32SuperBlock->dataBlockRange.begin, buffer, continousBlockLength);
            ERROR_GOTO_IF_ERROR(0);

            currentClusterIndex = nextClusterIndex;

            buffer += continousClusterLength * clusterSize;
            remainingFullClusterNum -= continousClusterLength;
        }

        remainByteNum %= clusterSize;
    }

    if (remainByteNum > 0) {
        DEBUG_ASSERT_SILENT(currentClusterIndex != (Uint32)INVALID_INDEX);
        blockDevice_readBlocks(targetBlockDevice, currentClusterIndex * BPB->sectorPerCluster + fat32SuperBlock->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);

        memory_memcpy(buffer, clusterBuffer, remainByteNum);

        remainByteNum = 0;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __fat32_iNode_doWriteData(iNode* inode, Index64 begin, const void* buffer, Size byteN, void* clusterBuffer) {
    SuperBlock* superBlock = inode->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    
    fsNode* node = inode->fsNode;
    FAT32Inode* fat32Inode = HOST_POINTER(inode, FAT32Inode, inode);

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;

    DEBUG_ASSERT_SILENT(node->type == FS_ENTRY_TYPE_FILE || node->type == FS_ENTRY_TYPE_DIRECTORY);
    DEBUG_ASSERT_SILENT(device_isBlockDevice(targetDevice));

    FAT32BPB* BPB = fat32SuperBlock->BPB;
    Size clusterSize = POWER_2(targetDevice->granularity) * BPB->sectorPerCluster;

    Index64 currentClusterIndex = fat32_getCluster(fat32SuperBlock, fat32Inode->firstCluster, begin / clusterSize);
    if (currentClusterIndex == (Uint32)INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size remainByteNum = byteN;
    if (begin % clusterSize != 0) {
        Index64 offsetInCluster = begin % clusterSize;
        blockDevice_readBlocks(targetBlockDevice, currentClusterIndex * BPB->sectorPerCluster + fat32SuperBlock->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);
        
        Size byteReadN = algorithms_umin64(remainByteNum, clusterSize - offsetInCluster);
        memory_memcpy(clusterBuffer + offsetInCluster, buffer, byteReadN);
        
        blockDevice_writeBlocks(targetBlockDevice, currentClusterIndex * BPB->sectorPerCluster + fat32SuperBlock->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);

        currentClusterIndex = fat32_getCluster(fat32SuperBlock, currentClusterIndex, 1);

        buffer += byteReadN;
        remainByteNum -= byteReadN;
    }

    if (remainByteNum >= clusterSize) {
        Size remainingFullClusterNum = remainByteNum / clusterSize;
        while (remainingFullClusterNum > 0) {
            DEBUG_ASSERT_SILENT(currentClusterIndex != (Uint32)INVALID_INDEX);

            Size continousClusterLength = 0;
            Index32 nextClusterIndex = fat32_stepCluster(fat32SuperBlock, currentClusterIndex, remainingFullClusterNum, &continousClusterLength);
            DEBUG_ASSERT_SILENT(VALUE_WITHIN(0, remainingFullClusterNum, continousClusterLength, <, <=));

            Size continousBlockLength = continousClusterLength * BPB->sectorPerCluster;
            blockDevice_writeBlocks(targetBlockDevice, currentClusterIndex * BPB->sectorPerCluster + fat32SuperBlock->dataBlockRange.begin, buffer, continousBlockLength);
            ERROR_GOTO_IF_ERROR(0);

            currentClusterIndex = nextClusterIndex;

            buffer += continousClusterLength * clusterSize;
            remainingFullClusterNum -= continousClusterLength;
        }

        remainByteNum %= clusterSize;
    }

    if (remainByteNum > 0) {
        DEBUG_ASSERT_SILENT(currentClusterIndex != (Uint32)INVALID_INDEX);
        blockDevice_readBlocks(targetBlockDevice, currentClusterIndex * BPB->sectorPerCluster + fat32SuperBlock->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);
        
        memory_memcpy(clusterBuffer, buffer, remainByteNum);

        blockDevice_writeBlocks(targetBlockDevice, currentClusterIndex * BPB->sectorPerCluster + fat32SuperBlock->dataBlockRange.begin, clusterBuffer, BPB->sectorPerCluster);
        ERROR_GOTO_IF_ERROR(0);
        
        remainByteNum = 0;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __fat32_iNode_resize(iNode* inode, Size newSizeInByte) {
    fsNode* fsNode = inode->fsNode;
    if (fsNode->type == FS_ENTRY_TYPE_DIRECTORY) {
        DEBUG_ASSERT_SILENT(inode->sizeInByte == 0);
    }

    Index32 freeClusterChain = INVALID_INDEX;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(inode->superBlock, FAT32SuperBlock, superBlock);
    FAT32BPB* BPB = fat32SuperBlock->BPB;
    Device* superBlockDevice = &inode->superBlock->blockDevice->device;
    FAT32Inode* fat32Inode = HOST_POINTER(inode, FAT32Inode, inode);

    Size newSizeInCluster = DIVIDE_ROUND_UP(DIVIDE_ROUND_UP_SHIFT(newSizeInByte, superBlockDevice->granularity), BPB->sectorPerCluster), oldSizeInCluster = DIVIDE_ROUND_UP(inode->sizeInBlock, BPB->sectorPerCluster);
    if (newSizeInByte == 0) {
        newSizeInCluster = 1;
    }
    
    if (newSizeInCluster < oldSizeInCluster) {
        Index32 tail = fat32_getCluster(fat32SuperBlock, fat32Inode->firstCluster, newSizeInCluster - 1);
        if (tail == INVALID_INDEX) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        Index32 cut = fat32_cutClusterChain(fat32SuperBlock, tail);
        fat32_freeClusterChain(fat32SuperBlock, cut);
    } else if (newSizeInCluster > oldSizeInCluster) {
        freeClusterChain = fat32_allocateClusterChain(fat32SuperBlock, newSizeInCluster - oldSizeInCluster);
        if (freeClusterChain == INVALID_INDEX) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        Index32 tail = fat32_getCluster(fat32SuperBlock, fat32Inode->firstCluster, oldSizeInCluster - 1);
        if (freeClusterChain == INVALID_INDEX) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        fat32_insertClusterChain(fat32SuperBlock, tail, freeClusterChain);
    }

    inode->sizeInBlock = newSizeInCluster * BPB->sectorPerCluster;
    inode->sizeInByte = newSizeInByte;

    return;
    ERROR_FINAL_BEGIN(0);
    if (freeClusterChain != INVALID_INDEX) {
        fat32_freeClusterChain(fat32SuperBlock, freeClusterChain);
    }
}

static void __fat32_iNode_iterateDirectoryEntries(iNode* inode, iNodeOperationIterateDirectoryEntryFunc func, Object arg, void* ret) {
    void* clusterBuffer = NULL, * entriesBuffer = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32BPB* BPB = fat32SuperBlock->BPB;

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = memory_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = memory_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    String entryName;
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    iNodeAttribute inodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    Size entriesLength = 0;
    DirectoryEntry directoryEntry;
    while (true) {
        __fat32_iNode_doReadData(inode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }
        
        entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_iNode_doReadData(inode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        
        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &inodeAttribute, &firstCluster, &size);
        fsEntryType type = TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY) ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE;

        FAT32NodeMetadata* metadata = fat32SuperBlock_getMetadataFromFirstCluster(fat32SuperBlock, firstCluster);
        if (metadata == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_CLEAR();
        }

        directoryEntry.name = entryName.data;
        directoryEntry.type = type;
        directoryEntry.inodeID = FAT32_NODE_METADATA_GET_INODE_ID(metadata);
        directoryEntry.size = size;
        directoryEntry.deviceID = DIRECTORY_ENTRY_DEVICE_ID_ANY;

        if (func(&directoryEntry, arg, ret)) {
            break;
        }
        ERROR_GOTO_IF_ERROR(0);

        currentPointer += entriesLength;
    }

    memory_free(clusterBuffer);
    memory_free(entriesBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        memory_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        memory_free(entriesBuffer);
    }
    return;
}

static void __fat32_iNode_addDirectoryEntry(iNode* inode, ConstCstring name, fsEntryType type, iNodeAttribute* attr, ID deviceID) {
    if (!(type == FS_ENTRY_TYPE_DIRECTORY || type == FS_ENTRY_TYPE_FILE)) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }
    void* clusterBuffer = NULL, * entriesBuffer = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32BPB* BPB = fat32SuperBlock->BPB;

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = memory_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = memory_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    bool found = false;
    String entryName;
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    iNodeAttribute inodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    Size entriesLength = 0;
    while (!found) {
        __fat32_iNode_doReadData(inode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }

        entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_iNode_doReadData(inode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);

        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &inodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(name, entryName.data) == 0 && (type == FS_ENTRY_TYPE_DIRECTORY) == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {            
            found = true;
            break;
        }
        
        currentPointer += entriesLength;
    }

    if (found) {
        ERROR_THROW(ERROR_ID_ALREADY_EXIST, 0);
    }

    Index32 newFirstCluster = fat32SuperBlock_createFirstCluster(fat32SuperBlock);
    if (newFirstCluster == INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    //TODO: Add '.' and '..' for directory

    entriesLength = fat32_directoryEntry_initEntries(entriesBuffer, name, type, attr, newFirstCluster, 0);
    ERROR_GOTO_IF_ERROR(0);

    iNode_rawResize(inode, inode->sizeInByte + entriesLength);
    ERROR_GOTO_IF_ERROR(0);

    __fat32_iNode_doWriteData(inode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    currentPointer += entriesLength;

    __fat32_iNode_doWriteData(inode, currentPointer, &_fat32_iNode_directoryTail, sizeof(FAT32DirectoryEntry), clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);

    memory_free(clusterBuffer);
    memory_free(entriesBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        memory_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        memory_free(entriesBuffer);
    }
}

static void __fat32_iNode_removeDirectoryEntry(iNode* inode, ConstCstring name, bool isDirectory) {
    void* clusterBuffer = NULL, * entriesBuffer = NULL, * remainingData = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32BPB* BPB = fat32SuperBlock->BPB;

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = memory_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = memory_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    bool found = false;
    String entryName;
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    iNodeAttribute inodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    Size entriesLength = 0;
    while (!found) {
        __fat32_iNode_doReadData(inode, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }

        entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_iNode_doReadData(inode, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);

        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &inodeAttribute, &firstCluster, &size);
        if (cstring_strcmp(name, entryName.data) == 0 && isDirectory == TEST_FLAGS(attribute, FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY)) {
            found = true;
            break;
        }

        currentPointer += entriesLength;
    }

    if (!found) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }

    fat32SuperBlock_unregisterMetadata(fat32SuperBlock, firstCluster);
    ERROR_GOTO_IF_ERROR(0);

    //TODO: unregister metadata here

    DEBUG_ASSERT_SILENT(entriesLength > 0);
    DEBUG_ASSERT_SILENT(currentPointer + entriesLength < inode->sizeInByte);
    Size remainingDataSize = inode->sizeInByte - currentPointer - entriesLength;
    remainingData = memory_allocate(remainingDataSize);
    __fat32_iNode_doReadData(inode, currentPointer + entriesLength, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    __fat32_iNode_doWriteData(inode, currentPointer, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    memory_free(remainingData);
    remainingData = NULL;
    
    iNode_rawResize(inode, inode->sizeInByte - entriesLength);
    ERROR_GOTO_IF_ERROR(0);
    
    memory_free(clusterBuffer);
    memory_free(entriesBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        memory_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        memory_free(entriesBuffer);
    }

    if (remainingData != NULL) {
        memory_free(remainingData);
    }
}

static void __fat32_iNode_renameDirectoryEntry(iNode* inode, fsNode* entry, iNode* moveTo, ConstCstring newName) {
    void* clusterBuffer = NULL, *entriesBuffer = NULL, * transplantEntriesBuffer = NULL, * remainingData = NULL;
    
    SuperBlock* superBlock = inode->superBlock;
    FAT32SuperBlock* fat32SuperBlock = HOST_POINTER(superBlock, FAT32SuperBlock, superBlock);
    FAT32BPB* BPB = fat32SuperBlock->BPB;

    BlockDevice* targetBlockDevice = superBlock->blockDevice;
    Device* targetDevice = &targetBlockDevice->device;
    
    clusterBuffer = memory_allocate(BPB->sectorPerCluster * POWER_2(targetDevice->granularity));
    if (clusterBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    entriesBuffer = memory_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (entriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    transplantEntriesBuffer = memory_allocate(FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE);
    if (transplantEntriesBuffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    //Remove from inode directory

    bool found = false;
    String entryName;
    string_initStruct(&entryName);
    ERROR_GOTO_IF_ERROR(0);
    Flags8 attribute = EMPTY_FLAGS;
    iNodeAttribute inodeAttribute;
    Index32 firstCluster = 0;
    Size size = 0;
    Index64 currentPointer = 0;
    Size entriesLength = 0;
    while (!found) {
        __fat32_iNode_doReadData(inode, currentPointer, transplantEntriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }

        entriesLength = fat32_directoryEntry_getEntriesLength(transplantEntriesBuffer);
        __fat32_iNode_doReadData(inode, currentPointer, transplantEntriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);

        fat32_directoryEntry_parse(transplantEntriesBuffer, &entryName, &attribute, &inodeAttribute, &firstCluster, &size);
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
    DEBUG_ASSERT_SILENT(currentPointer + entriesLength < inode->sizeInByte);
    Size remainingDataSize = inode->sizeInByte - currentPointer - entriesLength;
    remainingData = memory_allocate(remainingDataSize);
    __fat32_iNode_doReadData(inode, currentPointer + entriesLength, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    __fat32_iNode_doWriteData(inode, currentPointer, remainingData, remainingDataSize, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    memory_free(remainingData);
    remainingData = NULL;
    
    iNode_rawResize(inode, inode->sizeInByte - entriesLength);
    ERROR_GOTO_IF_ERROR(0);

    //Write to moveTo directory

    currentPointer = 0;
    Size transplantEntriesLength = entriesLength;
    entriesLength = 0;
    found = false;
    while (!found) {
        __fat32_iNode_doReadData(moveTo, currentPointer, entriesBuffer, sizeof(FAT32UnknownTypeEntry), clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);
        if (fat32_directoryEntry_isEnd(entriesBuffer)) {
            break;
        }

        entriesLength = fat32_directoryEntry_getEntriesLength(entriesBuffer);
        __fat32_iNode_doReadData(moveTo, currentPointer, entriesBuffer, entriesLength, clusterBuffer);
        ERROR_GOTO_IF_ERROR(0);

        fat32_directoryEntry_parse(entriesBuffer, &entryName, &attribute, &inodeAttribute, &firstCluster, &size);
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
    iNode_rawResize(moveTo, moveTo->sizeInByte + entriesLength);
    ERROR_GOTO_IF_ERROR(0);

    __fat32_iNode_doWriteData(moveTo, currentPointer, transplantEntriesBuffer, entriesLength, clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);
    currentPointer += entriesLength;

    __fat32_iNode_doWriteData(inode, currentPointer, &_fat32_iNode_directoryTail, sizeof(FAT32DirectoryEntry), clusterBuffer);
    ERROR_GOTO_IF_ERROR(0);

    fsNode_transplant(entry, moveTo->fsNode);
    ERROR_GOTO_IF_ERROR(0);

    memory_free(clusterBuffer);
    memory_free(entriesBuffer);
    memory_free(transplantEntriesBuffer);

    return;
    ERROR_FINAL_BEGIN(0);
    if (clusterBuffer != NULL) {
        memory_free(clusterBuffer);
    }

    if (entriesBuffer != NULL) {
        memory_free(entriesBuffer);
    }

    if (transplantEntriesBuffer != NULL) {
        memory_free(transplantEntriesBuffer);
    }

    if (remainingData != NULL) {
        memory_free(remainingData);
    }
}