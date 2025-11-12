#include<fs/ext2/vnode.h>

#include<devices/blockDevice.h>
#include<fs/ext2/ext2.h>
#include<fs/ext2/inode.h>
#include<fs/vnode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<system/pageTable.h>

typedef struct __EXT2vnodeIterateFuncIO __EXT2vnodeIterateFuncIO;
typedef struct __EXT2directoryEntry __EXT2directoryEntry;

typedef struct __EXT2vnodeIterateFuncIO {
    BlockDevice* device;
    Size blockDeviceSize;
    void* currentBuffer;
} __EXT2vnodeIterateFuncIO;

typedef struct __EXT2directoryEntry {
    Index32 inodeID;
    Uint16 recordLength;
    Uint8 nameLength;
#define __EXT2_DIRECTORY_ENTRY_UNKNOWN          0
#define __EXT2_DIRECTORY_ENTRY_REGULAR          1
#define __EXT2_DIRECTORY_ENTRY_DIRECTORY        2
#define __EXT2_DIRECTORY_ENTRY_CHARACTER_DEVICE 3
#define __EXT2_DIRECTORY_ENTRY_BLOCK_DEVICE     4
#define __EXT2_DIRECTORY_ENTRY_FIFO             5
#define __EXT2_DIRECTORY_ENTRY_SOCKET           6
#define __EXT2_DIRECTORY_ENTRY_SYMBOLIC_LINK    7
    Uint8 type;
    char name[0];
} __EXT2directoryEntry;

static inline fsEntryType __ext2_directoryEntry_convertTypeFS(Uint8 ext2type) {
    switch (ext2type) {
        case __EXT2_DIRECTORY_ENTRY_UNKNOWN: {
            return FS_ENTRY_TYPE_DUMMY;
        }
        case __EXT2_DIRECTORY_ENTRY_REGULAR: {
            return FS_ENTRY_TYPE_FILE;
        }
        case __EXT2_DIRECTORY_ENTRY_DIRECTORY: {
            return FS_ENTRY_TYPE_DIRECTORY;
        }
        case __EXT2_DIRECTORY_ENTRY_CHARACTER_DEVICE:
        case __EXT2_DIRECTORY_ENTRY_BLOCK_DEVICE: {
            return FS_ENTRY_TYPE_DEVICE;
        }
        default: {
            break;
        }
    }
    return FS_ENTRY_TYPE_DUMMY;
}

static inline Uint8 __ext2_directoryEntry_convertTypeExt2(fsEntryType fsType) {
    switch (fsType) {
        case FS_ENTRY_TYPE_DUMMY: {
            return __EXT2_DIRECTORY_ENTRY_UNKNOWN;
        }
        case FS_ENTRY_TYPE_FILE: {
            return __EXT2_DIRECTORY_ENTRY_REGULAR;
        }
        case FS_ENTRY_TYPE_DIRECTORY: {
            return __EXT2_DIRECTORY_ENTRY_DIRECTORY;
        }
        case FS_ENTRY_TYPE_DEVICE: {
            return __EXT2_DIRECTORY_ENTRY_BLOCK_DEVICE;
        }
        default: {
            break;
        }
    }
    return __EXT2_DIRECTORY_ENTRY_UNKNOWN;
}

static void __ext2_vNode_readData(vNode* vnode, Index64 begin, void* buffer, Size byteN);

static void __ext2_vNode_readDataIterateFunc(Index32 realIndex, void* args);

static void __ext2_vNode_writeData(vNode* vnode, Index64 begin, const void* buffer, Size byteN);

static void __ext2_vNode_writeDataIterateFunc(Index32 realIndex, void* args);

static void __ext2_vNode_resize(vNode* vnode, Size newSizeInByte);

static Index64 __ext2_vNode_addDirectoryEntry(vNode* vnode, DirectoryEntry* entry, FSnodeAttribute* attr);

static void __ext2_vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory);

static void __ext2_vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName);

static void __ext2_vNode_readDirectoryEntries(vNode* vnode);

static vNodeOperations _ext2_vNodeOperations = {
    .readData                   = __ext2_vNode_readData,
    .writeData                  = __ext2_vNode_writeData,
    .resize                     = __ext2_vNode_resize,
    .addDirectoryEntry          = __ext2_vNode_addDirectoryEntry,
    .removeDirectoryEntry       = __ext2_vNode_removeDirectoryEntry,
    .renameDirectoryEntry       = __ext2_vNode_renameDirectoryEntry,
    .readDirectoryEntries       = __ext2_vNode_readDirectoryEntries
};

vNodeOperations* ext2_vNode_getOperations() {
    return &_ext2_vNodeOperations;
}

static void __ext2_vNode_readData(vNode* vnode, Index64 begin, void* buffer, Size byteN) {
    EXT2vnode* ext2vnode = HOST_POINTER(vnode, EXT2vnode, vnode);
    EXT2fscore* ext2fscore = HOST_POINTER(vnode->fscore, EXT2fscore, fscore); 
    
    EXT2SuperBlock* superblock = ext2fscore->superBlock;
    EXT2inode* inode = &ext2vnode->inode;

    BlockDevice* blockDevice = ext2fscore->fscore.blockDevice;
    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift), blockDeviceSize = blockSize / POWER_2(blockDevice->device.granularity);
    Index32 inBlockOffset = begin % blockSize;
    Index32 currentIndex = begin;
    void* currentBuffer = buffer;
    Size remainingN = byteN;

    Uint8 blockBuffer[blockSize];
    memory_memset(blockBuffer, 0, blockSize);

    __EXT2vnodeIterateFuncIO args = {
        .device = blockDevice,
        .blockDeviceSize = blockDeviceSize,
        .currentBuffer = blockBuffer
    };
    
    if (inBlockOffset != 0) {
        Size inBlockN = blockSize - inBlockOffset;
        ext2Inode_iterateBlockRange(inode, ext2fscore, currentIndex / blockSize, 1, __ext2_vNode_readDataIterateFunc, &args);
        memory_memcpy(currentBuffer, blockBuffer + inBlockOffset, inBlockN);

        currentIndex += inBlockN;
        currentBuffer += inBlockN;
        remainingN -= inBlockN;
    }

    args.currentBuffer = currentBuffer;

    if (remainingN >= blockSize) {
        Size blockN = remainingN / blockSize;

        ext2Inode_iterateBlockRange(inode, ext2fscore, currentIndex / blockSize, blockN, __ext2_vNode_readDataIterateFunc, &args);

        currentIndex += blockN * blockSize;
        currentBuffer += blockN * blockSize;
        remainingN -= blockN * blockSize;
    }

    args.currentBuffer = blockBuffer;

    if (remainingN != 0) {
        ext2Inode_iterateBlockRange(inode, ext2fscore, currentIndex / blockSize, 1, __ext2_vNode_readDataIterateFunc, &args);
        memory_memcpy(currentBuffer, blockBuffer, remainingN);

        currentIndex += remainingN;
        currentBuffer += remainingN;
        remainingN -= remainingN;
    }
}

static void __ext2_vNode_readDataIterateFunc(Index32 blockIndex, void* args) {
    __EXT2vnodeIterateFuncIO* ioArgs = (__EXT2vnodeIterateFuncIO*)args;
    
    blockDevice_readBlocks(ioArgs->device, blockIndex * ioArgs->blockDeviceSize, ioArgs->currentBuffer, ioArgs->blockDeviceSize);
    ioArgs->currentBuffer += ioArgs->blockDeviceSize * POWER_2(ioArgs->device->device.granularity);
}

static void __ext2_vNode_writeData(vNode* vnode, Index64 begin, const void* buffer, Size byteN) {
    EXT2vnode* ext2vnode = HOST_POINTER(vnode, EXT2vnode, vnode);
    EXT2fscore* ext2fscore = HOST_POINTER(vnode->fscore, EXT2fscore, fscore); 
    
    EXT2SuperBlock* superblock = ext2fscore->superBlock;
    EXT2inode* inode = &ext2vnode->inode;

    BlockDevice* blockDevice = ext2fscore->fscore.blockDevice;
    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift), blockDeviceSize = blockSize / POWER_2(blockDevice->device.granularity);
    Index32 inBlockOffset = begin % blockSize;
    Index32 currentIndex = begin;
    void* currentBuffer = (void*)buffer;
    Size remainingN = byteN;

    Uint8 blockBuffer[blockSize];
    memory_memset(blockBuffer, 0, blockSize);

    __EXT2vnodeIterateFuncIO args = {
        .device = blockDevice,
        .blockDeviceSize = blockDeviceSize,
        .currentBuffer = blockBuffer
    };
    
    if (inBlockOffset != 0) {
        Size inBlockN = blockSize - inBlockOffset;
        memory_memcpy(blockBuffer + inBlockOffset, currentBuffer, inBlockN);
        ext2Inode_iterateBlockRange(inode, ext2fscore, currentIndex / blockSize, 1, __ext2_vNode_writeDataIterateFunc, &args);

        currentIndex += inBlockN;
        currentBuffer += inBlockN;
        remainingN -= inBlockN;
    }

    args.currentBuffer = currentBuffer;

    if (remainingN >= blockSize) {
        Size blockN = remainingN / blockSize;

        ext2Inode_iterateBlockRange(inode, ext2fscore, currentIndex / blockSize, blockN, __ext2_vNode_writeDataIterateFunc, &args);

        currentIndex += blockN * blockSize;
        currentBuffer += blockN * blockSize;
        remainingN -= blockN * blockSize;
    }

    args.currentBuffer = blockBuffer;

    if (remainingN != 0) {
        memory_memcpy(blockBuffer, currentBuffer, remainingN);
        ext2Inode_iterateBlockRange(inode, ext2fscore, currentIndex / blockSize, 1, __ext2_vNode_writeDataIterateFunc, &args);

        currentIndex += remainingN;
        currentBuffer += remainingN;
        remainingN -= remainingN;
    }
}

static void __ext2_vNode_writeDataIterateFunc(Index32 blockIndex, void* args) {
    __EXT2vnodeIterateFuncIO* ioArgs = (__EXT2vnodeIterateFuncIO*)args;
    
    blockDevice_writeBlocks(ioArgs->device, blockIndex * ioArgs->blockDeviceSize, ioArgs->currentBuffer, ioArgs->blockDeviceSize);
    ioArgs->currentBuffer += ioArgs->blockDeviceSize * POWER_2(ioArgs->device->device.granularity);
}

static void __ext2_vNode_resize(vNode* vnode, Size newSizeInByte) {
    EXT2vnode* ext2vnode = HOST_POINTER(vnode, EXT2vnode, vnode);
    EXT2fscore* ext2fscore = HOST_POINTER(vnode->fscore, EXT2fscore, fscore); 
    
    EXT2SuperBlock* superblock = ext2fscore->superBlock;
    EXT2inode* inode = &ext2vnode->inode;

    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);
    Size newSizeInBlock = DIVIDE_ROUND_UP(newSizeInByte, blockSize), oldSizeInBlock = DIVIDE_ROUND_UP(vnode->tokenSpaceSize, blockSize);
    Size deviceBlockNum = blockSize / POWER_2(ext2fscore->fscore.blockDevice->device.granularity);
    DEBUG_ASSERT_SILENT(oldSizeInBlock == DIVIDE_ROUND_UP(inode->sectorCnt, deviceBlockNum));
    if (newSizeInBlock < oldSizeInBlock) {
        ext2Inode_truncateTable(inode, ext2fscore, oldSizeInBlock, newSizeInBlock);
    } else if (newSizeInBlock > oldSizeInBlock) {
        ext2Inode_expandTable(inode, vnode->vnodeID, ext2fscore, oldSizeInBlock, newSizeInBlock);
    }
    inode->sectorCnt = newSizeInBlock * deviceBlockNum;
}

static Index64 __ext2_vNode_addDirectoryEntry(vNode* vnode, DirectoryEntry* entry, FSnodeAttribute* attr) { //TODO: NOT TESTED YET
    EXT2vnode* ext2vnode = HOST_POINTER(vnode, EXT2vnode, vnode);
    EXT2fscore* ext2fscore = HOST_POINTER(vnode->fscore, EXT2fscore, fscore); 
    
    EXT2SuperBlock* superblock = ext2fscore->superBlock;
    EXT2inode* inode = &ext2vnode->inode;
    DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(vnode->fsNode);

    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);
    Uint8 granularity = ext2fscore->fscore.blockDevice->device.granularity;
    Size deviceBlockNum = blockSize / POWER_2(granularity);
    Size blockNum = inode->sectorCnt / deviceBlockNum;

    Index32 indexInBuffer = blockSize;

    Uint8 deviceBlockBuffer[POWER_2(granularity)];
    FSnodeAttribute fsnodeAttribute;

    Size bufferSize = 2 * blockSize;
    Uint8* buffer = NULL;
    buffer = mm_allocatePages(DIVIDE_ROUND_UP(bufferSize, PAGE_SIZE));
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size nameLength = cstring_strlen(entry->name);
    Size requiredSpaceLength = ALIGN_UP(sizeof(__EXT2directoryEntry) + nameLength, 4);

    Index32 blockGroupIndex = ext2SuperBlock_inodeID2BlockGroupIndex(superblock, vnode->vnodeID);
    Index32 newInodeID = ext2fscore_allocateInode(ext2fscore, blockGroupIndex, entry->type == FS_ENTRY_TYPE_DIRECTORY);
    Index32 currentIndex = 0;
    Index32 lastIndex = 0;
    __EXT2directoryEntry* lastEntry = NULL;
    Size lastEntryActualSize = 0;
    bool found = false;

    for (Index32 i = 0; i < blockNum; ++i) {
        vNode_rawReadData(vnode, i * blockSize, &buffer[blockSize], blockSize);
        ERROR_GOTO_IF_ERROR(0);

        while (!found) {    //TODO: Not covering the case of first entry deleted
            __EXT2directoryEntry* currentEntry = (__EXT2directoryEntry*)(buffer + indexInBuffer);
            
            if (indexInBuffer + 4 >= bufferSize || indexInBuffer + currentEntry->recordLength > bufferSize) {   //It works for EXT2 directory tail mechanism
                memory_memcpy(buffer, buffer + blockSize, blockSize);
                indexInBuffer -= blockSize;
                break;
            }   //TODO: Not considering record length is large (Larger then two blocks)

            Size entryActualSize = ALIGN_UP(sizeof(__EXT2directoryEntry) + currentEntry->nameLength, 4);
            Size holeSize = currentEntry->recordLength - entryActualSize;

            if (holeSize >= requiredSpaceLength) {
                __EXT2directoryEntry* newEntry = (__EXT2directoryEntry*)((void*)entry + entryActualSize);

                newEntry->inodeID = newInodeID;
                newEntry->recordLength = holeSize;
                newEntry->nameLength = nameLength;
                newEntry->type = __ext2_directoryEntry_convertTypeExt2(entry->type);
                memory_memcpy(newEntry->name, entry->name, nameLength);
                
                currentEntry->recordLength = entryActualSize;

                vNode_rawWriteData(vnode, currentIndex, currentEntry, currentEntry->recordLength + requiredSpaceLength);

                found = true;
            }

            indexInBuffer += currentEntry->recordLength;
            lastIndex = currentIndex;
            currentIndex += currentEntry->recordLength;
            lastEntry = currentEntry;
            lastEntryActualSize = entryActualSize;
        }

        if (found) {
            break;
        }
    }

    if (!found) {
        Size newTailEntriesSize = lastEntryActualSize + requiredSpaceLength;
        vNode_rawResize(vnode, lastIndex + newTailEntriesSize);
        DEBUG_ASSERT_SILENT(lastEntry != NULL);
        void* newTailEntries = mm_allocate(newTailEntriesSize);
        memory_memcpy(newTailEntries, lastEntry, lastEntryActualSize);

        __EXT2directoryEntry* lastEntry = (__EXT2directoryEntry*)newTailEntries;
        __EXT2directoryEntry* newEntry = (__EXT2directoryEntry*)((void*)entry + lastEntryActualSize);

        newEntry->inodeID = newInodeID;
        newEntry->recordLength = blockSize - ((lastIndex + lastEntryActualSize) % (blockSize + 1));
        newEntry->nameLength = nameLength;
        newEntry->type = __ext2_directoryEntry_convertTypeExt2(entry->type);

        lastEntry->recordLength = lastEntryActualSize;

        vNode_rawWriteData(vnode, lastIndex, newTailEntries, newTailEntriesSize);

        mm_free(newTailEntries);
    }

    mm_freePages(buffer);
    return (Index64)newInodeID;

    ERROR_FINAL_BEGIN(0);

    if (buffer != NULL) {
        mm_freePages(buffer);
    }
}

static void __ext2_vNode_removeDirectoryEntry(vNode* vnode, ConstCstring name, bool isDirectory) {  //TODO: NOT TESTED YET
    EXT2vnode* ext2vnode = HOST_POINTER(vnode, EXT2vnode, vnode);
    EXT2fscore* ext2fscore = HOST_POINTER(vnode->fscore, EXT2fscore, fscore); 
    
    EXT2SuperBlock* superblock = ext2fscore->superBlock;
    EXT2inode* inode = &ext2vnode->inode;
    DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(vnode->fsNode);

    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);
    Uint8 granularity = ext2fscore->fscore.blockDevice->device.granularity;
    Size deviceBlockNum = blockSize / POWER_2(granularity);
    Size blockNum = inode->sectorCnt / deviceBlockNum;

    Index32 indexInBuffer = blockSize, lastIndexInBuffer = INVALID_INDEX32;
    Index32 currentIndex = 0, lastCurrentIndex = INVALID_INDEX32;
    Size lastEntryActualSize = 0, currentEntryActualSize = 0;

    Uint8 deviceBlockBuffer[POWER_2(granularity)];
    FSnodeAttribute fsnodeAttribute;

    Size bufferSize = 2 * blockSize;
    Uint8* buffer = NULL;
    buffer = mm_allocatePages(DIVIDE_ROUND_UP(bufferSize, PAGE_SIZE));
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    bool found = false;
    for (Index32 i = 0; i < blockNum; ++i) {
        vNode_rawReadData(vnode, i * blockSize, &buffer[blockSize], blockSize);
        ERROR_GOTO_IF_ERROR(0);

        while (!found) {    //TODO: Not covering the case of first entry deleted
            __EXT2directoryEntry* entry = (__EXT2directoryEntry*)(buffer + indexInBuffer);
            if (indexInBuffer + 4 >= bufferSize || indexInBuffer + entry->recordLength > bufferSize) {  //It works for EXT2 directory tail mechanism
                memory_memcpy(buffer, buffer + blockSize, blockSize);
                lastIndexInBuffer -= blockSize;
                indexInBuffer -= blockSize;
                break;
            }   //TODO: Not considering record length is large

            lastEntryActualSize = currentEntryActualSize;
            currentEntryActualSize = ALIGN_UP(sizeof(__EXT2directoryEntry) + entry->nameLength, 4);

            if (cstring_strncmp(entry->name, name, entry->nameLength) == 0 && isDirectory == (entry->type == __EXT2_DIRECTORY_ENTRY_DIRECTORY)) {
                entry->inodeID = 0;
                if (lastIndexInBuffer == INVALID_INDEX32) {
                    vNode_rawWriteData(vnode, 0, entry, currentEntryActualSize);
                } else {
                    DEBUG_ASSERT_SILENT(0 <= lastIndexInBuffer && lastIndexInBuffer < bufferSize);
                    __EXT2directoryEntry* lastEntry = (__EXT2directoryEntry*)(buffer + lastIndexInBuffer);
                    lastEntry->recordLength += entry->recordLength;
                    vNode_rawWriteData(vnode, lastCurrentIndex, entry, currentIndex - lastCurrentIndex + sizeof(__EXT2directoryEntry));
                }
                found = true;
                break;
            }

            lastIndexInBuffer = indexInBuffer;
            indexInBuffer += entry->recordLength;
            lastCurrentIndex = currentIndex;
            currentIndex += entry->recordLength;
        }

        if (found) {
            break;
        }
    }

    mm_freePages(buffer);
    return;

    ERROR_FINAL_BEGIN(0);

    if (buffer != NULL) {
        mm_freePages(buffer);
    }
}

static void __ext2_vNode_renameDirectoryEntry(vNode* vnode, fsNode* entry, vNode* moveTo, ConstCstring newName) {   //TODO: NOT TESTED YET
    EXT2vnode* ext2vnode = HOST_POINTER(vnode, EXT2vnode, vnode);
    EXT2fscore* ext2fscore = HOST_POINTER(vnode->fscore, EXT2fscore, fscore); 
    
    EXT2SuperBlock* superblock = ext2fscore->superBlock;
    EXT2inode* inode = &ext2vnode->inode;
    DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(vnode->fsNode);

    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);
    Uint8 granularity = ext2fscore->fscore.blockDevice->device.granularity;
    Size deviceBlockNum = blockSize / POWER_2(granularity);
    Size blockNum = inode->sectorCnt / deviceBlockNum;

    Index32 indexInBuffer = blockSize, lastIndexInBuffer = INVALID_INDEX32;
    Index32 currentIndex = 0, lastCurrentIndex = INVALID_INDEX32;
    Size lastEntryActualSize = 0, currentEntryActualSize = 0;

    Uint8 deviceBlockBuffer[POWER_2(granularity)];
    FSnodeAttribute fsnodeAttribute;

    Size bufferSize = 2 * blockSize;
    Uint8* buffer = NULL;
    void* movedEntry = NULL;
    buffer = mm_allocatePages(DIVIDE_ROUND_UP(bufferSize, PAGE_SIZE));
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size nameLength = cstring_strlen(newName);
    Size requiredSpaceLength = ALIGN_UP(sizeof(__EXT2directoryEntry) + nameLength, 4);
    movedEntry = mm_allocate(requiredSpaceLength);
    memory_memcpy(((__EXT2directoryEntry*)movedEntry)->name, newName, nameLength);

    bool found = false;
    for (Index32 i = 0; i < blockNum; ++i) {
        vNode_rawReadData(vnode, i * blockSize, &buffer[blockSize], blockSize);
        ERROR_GOTO_IF_ERROR(0);

        while (!found) {    //TODO: Not covering the case of first entry deleted
            __EXT2directoryEntry* currentEntry = (__EXT2directoryEntry*)(buffer + indexInBuffer);
            if (indexInBuffer + 4 >= bufferSize || indexInBuffer + currentEntry->recordLength > bufferSize) {  //It works for EXT2 directory tail mechanism
                memory_memcpy(buffer, buffer + blockSize, blockSize);
                lastIndexInBuffer -= blockSize;
                indexInBuffer -= blockSize;
                break;
            }   //TODO: Not considering record length is large

            lastEntryActualSize = currentEntryActualSize;
            currentEntryActualSize = ALIGN_UP(sizeof(__EXT2directoryEntry) + currentEntry->nameLength, 4);

            if (cstring_strncmp(currentEntry->name, entry->entry.name, currentEntry->nameLength) == 0 && entry->entry.type == __ext2_directoryEntry_convertTypeFS(currentEntry->type)) {
                memory_memcpy(movedEntry, currentEntry, sizeof(__EXT2directoryEntry));
                currentEntry->inodeID = 0;
                if (lastIndexInBuffer == INVALID_INDEX32) {
                    vNode_rawWriteData(vnode, 0, entry, currentEntryActualSize);
                } else {
                    DEBUG_ASSERT_SILENT(0 <= lastIndexInBuffer && lastIndexInBuffer < bufferSize);
                    __EXT2directoryEntry* lastEntry = (__EXT2directoryEntry*)(buffer + lastIndexInBuffer);
                    lastEntry->recordLength += currentEntry->recordLength;
                    vNode_rawWriteData(vnode, lastCurrentIndex, entry, currentIndex - lastCurrentIndex + sizeof(__EXT2directoryEntry));
                }
                found = true;
                break;
            }

            lastIndexInBuffer = indexInBuffer;
            indexInBuffer += currentEntry->recordLength;
            lastCurrentIndex = currentIndex;
            currentIndex += currentEntry->recordLength;
        }

        if (found) {
            break;
        }
    }

    if (!found) {
        mm_freePages(buffer);
        mm_free(movedEntry);
        return;
    }

    indexInBuffer = blockSize, lastIndexInBuffer = INVALID_INDEX32;
    currentIndex = 0, lastCurrentIndex = INVALID_INDEX32;
    lastEntryActualSize = 0, currentEntryActualSize = 0;

    __EXT2directoryEntry* lastEntry = NULL;
    found = false;

    for (Index32 i = 0; i < blockNum; ++i) {
        vNode_rawReadData(vnode, i * blockSize, &buffer[blockSize], blockSize);
        ERROR_GOTO_IF_ERROR(0);

        while (!found) {    //TODO: Not covering the case of first entry deleted
            __EXT2directoryEntry* currentEntry = (__EXT2directoryEntry*)(buffer + indexInBuffer);
            
            if (indexInBuffer + 4 >= bufferSize || indexInBuffer + currentEntry->recordLength > bufferSize) {   //It works for EXT2 directory tail mechanism
                memory_memcpy(buffer, buffer + blockSize, blockSize);
                indexInBuffer -= blockSize;
                break;
            }   //TODO: Not considering record length is large (Larger then two blocks)

            Size entryActualSize = ALIGN_UP(sizeof(__EXT2directoryEntry) + currentEntry->nameLength, 4);
            Size holeSize = currentEntry->recordLength - entryActualSize;

            if (holeSize >= requiredSpaceLength) {
                __EXT2directoryEntry* newEntry = (__EXT2directoryEntry*)((void*)entry + entryActualSize);
                memory_memcpy(newEntry, movedEntry, requiredSpaceLength);
                newEntry->recordLength = holeSize;
                
                currentEntry->recordLength = entryActualSize;

                vNode_rawWriteData(vnode, currentIndex, currentEntry, currentEntry->recordLength + requiredSpaceLength);

                found = true;
            }

            indexInBuffer += currentEntry->recordLength;
            lastCurrentIndex = currentIndex;
            currentIndex += currentEntry->recordLength;
            lastEntry = currentEntry;
            lastEntryActualSize = entryActualSize;
        }

        if (found) {
            break;
        }
    }

    if (!found) {
        Size newTailEntriesSize = lastEntryActualSize + requiredSpaceLength;
        vNode_rawResize(vnode, lastCurrentIndex + newTailEntriesSize);
        DEBUG_ASSERT_SILENT(lastEntry != NULL);
        void* newTailEntries = mm_allocate(newTailEntriesSize);
        memory_memcpy(newTailEntries, lastEntry, lastEntryActualSize);

        __EXT2directoryEntry* lastEntry = (__EXT2directoryEntry*)newTailEntries;
        __EXT2directoryEntry* newEntry = (__EXT2directoryEntry*)((void*)entry + lastEntryActualSize);

        memory_memcpy(newEntry, movedEntry, requiredSpaceLength);
        newEntry->recordLength = blockSize - ((lastCurrentIndex + lastEntryActualSize) % (blockSize + 1));

        lastEntry->recordLength = lastEntryActualSize;

        vNode_rawWriteData(vnode, lastCurrentIndex, newTailEntries, newTailEntriesSize);
    }

    mm_free(movedEntry);
    mm_freePages(buffer);
    return;

    ERROR_FINAL_BEGIN(0);

    if (movedEntry != NULL) {
        mm_free(movedEntry);
    }

    if (buffer != NULL) {
        mm_freePages(buffer);
    }
}

static void __ext2_vNode_readDirectoryEntries(vNode* vnode) {
    EXT2vnode* ext2vnode = HOST_POINTER(vnode, EXT2vnode, vnode);
    EXT2fscore* ext2fscore = HOST_POINTER(vnode->fscore, EXT2fscore, fscore); 
    
    EXT2SuperBlock* superblock = ext2fscore->superBlock;
    EXT2inode* inode = &ext2vnode->inode;
    DirFSnode* dirNode = FSNODE_GET_DIRFSNODE(vnode->fsNode);

    Size blockSize = EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);
    Uint8 granularity = ext2fscore->fscore.blockDevice->device.granularity;
    Size deviceBlockNum = blockSize / POWER_2(granularity);
    Size blockNum = inode->sectorCnt / deviceBlockNum;

    Index32 indexInBuffer = blockSize;

    Uint8 deviceBlockBuffer[POWER_2(granularity)];
    FSnodeAttribute fsnodeAttribute;

    Size bufferSize = 2 * blockSize;
    Uint8* buffer = NULL;
    buffer = mm_allocatePages(DIVIDE_ROUND_UP(bufferSize, PAGE_SIZE));
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    DEBUG_ASSERT_SILENT(dirNode->dirPart.childrenNum == FSNODE_DIR_PART_UNKNOWN_CHILDREN_NUM);
    dirNode->dirPart.childrenNum = 0;
    for (Index32 i = 0; i < blockNum; ++i) {
        vNode_rawReadData(vnode, i * blockSize, &buffer[blockSize], blockSize);
        ERROR_GOTO_IF_ERROR(0);

        while (true) {  //TODO: Not covering the case of first entry deleted
            __EXT2directoryEntry* entry = (__EXT2directoryEntry*)(buffer + indexInBuffer);
            if (indexInBuffer + 4 >= bufferSize || indexInBuffer + entry->recordLength > bufferSize) {  //It works for EXT2 directory tail mechanism
                memory_memcpy(buffer, buffer + blockSize, blockSize);
                indexInBuffer -= blockSize;
                break;
            }   //TODO: Not considering record length is large

            fsEntryType type = __ext2_directoryEntry_convertTypeFS(entry->type);
            DEBUG_ASSERT_SILENT(type != FS_ENTRY_TYPE_DUMMY);

            Index32 blockGroupID = ext2SuperBlock_inodeID2BlockGroupIndex(superblock, entry->inodeID);
            DEBUG_ASSERT_SILENT(blockGroupID < ext2fscore->blockGroupNum);
            EXT2blockGroupDescriptor* desc = &ext2fscore->blockGroupTables[blockGroupID];

            Index32 inodeIndexInBlockGroup = (entry->inodeID - 1) % superblock->blockGroupInodeNum;
            Index32 inodeBlockIndex = desc->inodeTableIndex + (inodeIndexInBlockGroup * superblock->iNodeSizeInByte) / EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);
            Index32 inodeBlockOffset = (inodeIndexInBlockGroup * superblock->iNodeSizeInByte) % EXT2_SUPERBLOCK_IN_STORAGE_GET_BLOCK_SIZE(superblock->blockSizeShift);

            Index32 inodeDeviceBlockIndex = ext2SuperBlock_blockIndexFS2device(superblock, granularity, inodeBlockIndex, inodeBlockOffset);
            Index32 inodeDeviceBlockOffset = ext2SuperBlock_blockOffsetFS2device(superblock, granularity, inodeBlockIndex, inodeBlockOffset);

            blockDevice_readBlocks(ext2fscore->fscore.blockDevice, inodeDeviceBlockIndex, deviceBlockBuffer, 1);
            ERROR_GOTO_IF_ERROR(0);

            EXT2inode* inode = (EXT2inode*)(deviceBlockBuffer + inodeDeviceBlockOffset);

            Size size = 0;
            if (type == FS_ENTRY_TYPE_FILE) {
                size = ((Size)inode->h32Size << 32) | inode->l32Size;
            }

            fsnodeAttribute.createTime = inode->createTime;
            fsnodeAttribute.lastAccessTime = inode->lastAccessTime;
            fsnodeAttribute.lastModifyTime = inode->modificationTime;

            DirectoryEntry newDirEntry = (DirectoryEntry) {
                .name = entry->name,
                .type = type,
                .mode = 0,
                .vnodeID = entry->inodeID,
                .size = size,
                .pointsTo = entry->inodeID
            };

            fsnode_create(&newDirEntry, entry->nameLength, &fsnodeAttribute, &dirNode->node);

            indexInBuffer += entry->recordLength;
        }
    }

    mm_freePages(buffer);
    return;

    ERROR_FINAL_BEGIN(0);

    if (buffer != NULL) {
        mm_freePages(buffer);
    }
}