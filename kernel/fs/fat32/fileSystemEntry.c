#include<fs/fat32/fileSystemEntry.h>

#include<algorithms.h>
#include<fs/fat32/fat32.h>
#include<fs/fat32/inode.h>
#include<fs/fileSystem.h>
#include<fs/fileSystemEntry.h>
#include<fs/fsutil.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/kMalloc.h>
#include<string.h>
#include<time/time.h>

typedef struct {
#define __FAT32_DIRECTORY_ENTRY_NAME_MAIN_LENGTH    8
#define __FAT32_DIRECTORY_ENTRY_NAME_EXT_LENGTH     3
#define __FAT32_DIRECTORY_ENTRY_NAME_LENGTH         (__FAT32_DIRECTORY_ENTRY_NAME_MAIN_LENGTH + __FAT32_DIRECTORY_ENTRY_NAME_EXT_LENGTH)
    char    name[__FAT32_DIRECTORY_ENTRY_NAME_LENGTH];
    Uint8   attribute;
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY FLAG8(0)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN    FLAG8(1)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_SYSTEM    FLAG8(2)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_VOLUME_ID FLAG8(3)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY FLAG8(4)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_ARCHIVE   FLAG8(5)
    Uint8   reserved;
    Uint8   createTimeTengthSec;    //Second in tength of second
    Uint16  createTime;             //5 bits hours, 6 bits minutes, 5 bits seconds(Multiply by 2)
    Uint16  createDate;             //7 bits year, 4 bit month, 5 bit day
    Uint16  lastAccessDate;         //Same format as create date
    Uint16  clusterBeginHigh;
    Uint16  lastModifyTime;         //Same format as create date
    Uint16  lastModifyDate;         //Same format as create time
    Uint16  clusterBeginLow;
    Uint32  size;
} __attribute__((packed)) __FAT32DirectoryEntry;

#define __FAT32_LONG_NAME_ENTRY_LEN1                5
#define __FAT32_LONG_NAME_ENTRY_LEN2                6
#define __FAT32_LONG_NAME_ENTRY_LEN3                2
#define __FAT32_LONG_NAME_ENTRY_LEN                 (__FAT32_LONG_NAME_ENTRY_LEN1 + __FAT32_LONG_NAME_ENTRY_LEN2 + __FAT32_LONG_NAME_ENTRY_LEN3)
#define __FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH 255

typedef struct {
    Uint8   order;
#define __FAT32_LONG_NAME_ENTRY_FLAG                FLAG8(6)
    Uint16  doubleBytes1[__FAT32_LONG_NAME_ENTRY_LEN1];
    Uint8   attribute;
#define __FAT32_LONG_NAME_ENTRY_ATTRIBUTE           (__FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY | __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN | __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_SYSTEM | __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_VOLUME_ID)
    Uint8   type;
    Uint8   checksum;
    Uint16  doubleBytes2[__FAT32_LONG_NAME_ENTRY_LEN2];
    Uint16  reserved;
    Uint16  doubleBytes3[__FAT32_LONG_NAME_ENTRY_LEN3];
} __attribute__((packed)) __FAT32LongNameEntry;

static Result __FAT32resize(FileSystemEntry* entry, Size newSizeInByte);

static Result __FAT32readChild(FileSystemEntry* directory, FileSystemEntryDescriptor* childDesc, Size* entrySizePtr);

static Result __FAT32addChild(FileSystemEntry* directory, FileSystemEntryDescriptor* childToAdd);

static Result __FAT32removeChild(FileSystemEntry* directory, FileSystemEntryIdentifier* childToRemove);

static Result __FAT32updateChild(FileSystemEntry* directory, FileSystemEntryIdentifier* oldChild, FileSystemEntryDescriptor* newChild);

static FileSystemEntryOperations _FAT32fileSystemEntryOperations = {
    .seek           = genericFileSystemEntrySeek,
    .read           = genericFileSystemEntryRead,
    .write          = genericFileSystemEntryWrite,
    .resize         = __FAT32resize,
    .readChild      = __FAT32readChild,
    .addChild       = __FAT32addChild,
    .removeChild    = __FAT32removeChild,
    .updateChild    = __FAT32updateChild
};

Result FAT32openFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry, FileSystemEntryDescriptor* entryDescriptor) {
    if (genericOpenFileSystemEntry(superBlock, entry, entryDescriptor) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    entry->operations = &_FAT32fileSystemEntryOperations;
    if (entry->descriptor->identifier.type == FILE_SYSTEM_ENTRY_TYPE_DIRECTOY) {
        Size directoryDataSize = 0, entrySize;
        Result res = RESULT_FAIL;
        rawFileSystemEntrySeek(entry, 0);
        while((res = rawFileSystemEntryReadChild(entry, NULL, &entrySize)) != RESULT_FAIL) {
            directoryDataSize += entrySize;
            rawFileSystemEntrySeek(entry, entry->pointer + entrySize);

            if (res == RESULT_SUCCESS) {
                break;
            }
        }

        if (res == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        entry->descriptor->dataRange.length = directoryDataSize;
    }

    return RESULT_SUCCESS;
}

static Result __FAT32resize(FileSystemEntry* entry, Size newSizeInByte) {
    iNode* iNode = entry->iNode;
    FAT32info* info = (FAT32info*)iNode->superBlock->specificInfo;
    FAT32BPB* BPB = info->BPB;
    FAT32iNodeInfo* iNodeInfo = (FAT32iNodeInfo*)iNode->specificInfo;
    Size newSizeInCluster = DIVIDE_ROUND_UP(DIVIDE_ROUND_UP_SHIFT(newSizeInByte, iNode->superBlock->device->bytePerBlockShift), BPB->sectorPerCluster), oldSizeInCluster = DIVIDE_ROUND_UP(iNode->sizeInBlock, BPB->sectorPerCluster);

    if (newSizeInCluster < oldSizeInCluster) {
        Index32 tail = FAT32getCluster(info, iNodeInfo->firstCluster, newSizeInCluster - 1);
        if (FAT32getClusterType(info, tail) != FAT32_CLUSTER_TYPE_ALLOCATERD || PTR_TO_VALUE(32, info->FAT + tail) == FAT32_END_OF_CLUSTER_CHAIN) {
            return RESULT_FAIL;
        }

        Index32 cut = FAT32cutClusterChain(info, tail);
        FAT32releaseClusterChain(info, cut);
    } else if (newSizeInCluster > oldSizeInCluster) {
        Index32 freeClusterChain = FAT32allocateClusterChain(info, newSizeInCluster - oldSizeInCluster);
        Index32 tail = FAT32getCluster(info, iNodeInfo->firstCluster, iNode->sizeInBlock / BPB->sectorPerCluster - 1);

        if (FAT32getClusterType(info, tail) != FAT32_CLUSTER_TYPE_ALLOCATERD || FAT32getClusterType(info, freeClusterChain) != FAT32_CLUSTER_TYPE_ALLOCATERD || PTR_TO_VALUE(32, info->FAT + tail) != FAT32_END_OF_CLUSTER_CHAIN) {
            return RESULT_FAIL;
        }

        FAT32insertClusterChain(info, tail, freeClusterChain);
    }

    iNode->sizeInBlock = newSizeInCluster * BPB->sectorPerCluster;
    entry->descriptor->dataRange.length = newSizeInByte;

    return RESULT_SUCCESS;
}

static Result __doFAT32readChild(FileSystemEntry* directory, FileSystemEntryDescriptor* childDesc, Size* entrySizePtr, char* buffer);

static Result __FAT32readChild(FileSystemEntry* directory, FileSystemEntryDescriptor* childDesc, Size* entrySizePtr) {
    void* buffer = allocateBuffer(BUFFER_SIZE_512);
    if (buffer == NULL) {
        return RESULT_FAIL;
    }

    Result res = __doFAT32readChild(directory, childDesc, entrySizePtr, buffer);
    releaseBuffer(buffer, BUFFER_SIZE_512);

    return res;
}

static void __FAT32convertDateFieldToRealTime(Uint16 date, RealTime* realTime);

static void __FAT32convertTimeFieldToRealTime(Uint16 time, RealTime* realTime);

static Uint16 __FAT32convertRealTimeToDateField(RealTime* realTime);

static Uint16 __FAT32convertRealTimeToTimeField(RealTime* realTime);

static Result __doFAT32readChild(FileSystemEntry* directory, FileSystemEntryDescriptor* childDesc, Size* entrySizePtr, char* buffer) {
    if (rawFileSystemEntryRead(directory, buffer, sizeof(__FAT32DirectoryEntry)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    FileSystemEntryDescriptorInitArgs args;
    if (buffer[0] == 0x00 || buffer[0] == 0xE5) {
        if (childDesc != NULL) {
            args = (FileSystemEntryDescriptorInitArgs) {
                .name       = NULL,
                .type       = FILE_SYSTEM_ENTRY_TYPE_DUMMY,
                .dataRange  = RANGE(FILE_SYSTEM_ENTRY_INVALID_POSITION, FILE_SYSTEM_ENTRY_INVALID_SIZE),
                .parent     = directory->descriptor,
                .flags      = EMPTY_FLAGS
            };

            initFileSystemEntryDescriptor(childDesc, &args);
        }

        if (buffer[0] != 0x00 && entrySizePtr != NULL) {
            *entrySizePtr = 32;
        }

        return buffer[0] == 0x00 ? RESULT_SUCCESS : RESULT_CONTINUE;
    }

    Uint8 longNameEntryNum = 0;
    if (((__FAT32LongNameEntry*)buffer)->attribute == __FAT32_LONG_NAME_ENTRY_ATTRIBUTE) {
        __FAT32LongNameEntry* longNameEntry = (__FAT32LongNameEntry*)(buffer);

        if (TEST_FLAGS_FAIL(longNameEntry->order, __FAT32_LONG_NAME_ENTRY_FLAG)) {
            return RESULT_FAIL;
        }

        longNameEntryNum = (Uint8)VAL_XOR(longNameEntry->order, __FAT32_LONG_NAME_ENTRY_FLAG);
    }

    Size entrySize = longNameEntryNum * sizeof(__FAT32LongNameEntry) + sizeof(__FAT32DirectoryEntry);

    if (childDesc != NULL) {
        if (rawFileSystemEntryRead(directory, buffer, entrySize) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        char nameBuffer[__FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH + 1];
        Uint8 nameLength = 0;
        __FAT32DirectoryEntry* directoryEntry = (__FAT32DirectoryEntry*)(buffer + entrySize - sizeof(__FAT32DirectoryEntry));

        if (longNameEntryNum == 0) {
            for (int i = 0; i < 8 && directoryEntry->name[i] != ' '; ++i) {
                nameBuffer[nameLength++] = directoryEntry->name[i];
            }

            if (directoryEntry->name[8] != ' ') {
                nameBuffer[nameLength++] = '.';
                for (int i = 8; i < 11 && directoryEntry->name[i] != ' '; ++i) {
                    nameBuffer[nameLength++] = directoryEntry->name[i];
                }
            }
        } else {
            for (int i = longNameEntryNum - 1; i >= 0; --i) {
                __FAT32LongNameEntry* longNameEntry = (__FAT32LongNameEntry*)(buffer + i * sizeof(__FAT32LongNameEntry));

                bool notEnd = true;
                for (int i = 0; i < __FAT32_LONG_NAME_ENTRY_LEN1 && (notEnd &= longNameEntry->doubleBytes1[i] != 0) && nameLength < __FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH; ++i) {
                    nameBuffer[nameLength++] = (char)longNameEntry->doubleBytes1[i];
                }

                for (int i = 0; i < __FAT32_LONG_NAME_ENTRY_LEN2 && (notEnd &= longNameEntry->doubleBytes2[i] != 0) && nameLength < __FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH; ++i) {
                    nameBuffer[nameLength++] = (char)longNameEntry->doubleBytes2[i];
                }

                for (int i = 0; i < __FAT32_LONG_NAME_ENTRY_LEN3 && (notEnd &= longNameEntry->doubleBytes3[i] != 0) && nameLength < __FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH; ++i) {
                    nameBuffer[nameLength++] = (char)longNameEntry->doubleBytes3[i];
                }

                if (nameLength == __FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH && notEnd) {
                    return RESULT_FAIL;
                }
            }
        }
        nameBuffer[nameLength]      = '\0';

        SuperBlock* superBlock      = directory->iNode->superBlock;
        FAT32info* info             = (FAT32info*)superBlock->specificInfo;
        FAT32BPB* BPB               = info->BPB;

        Index32 clusterBegin        = ((Uint32)directoryEntry->clusterBeginHigh << 16) | directoryEntry->clusterBeginLow;
        FileSystemEntryType type    = TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY) ? FILE_SYSTEM_ENTRY_TYPE_DIRECTOY : FILE_SYSTEM_ENTRY_TYPE_FILE;

        Uint64 createTime, lastAccessTime, lastModifyTime;
        RealTime realTime;
        Timestamp timestamp;
        __FAT32convertDateFieldToRealTime(directoryEntry->createDate, &realTime);
        __FAT32convertTimeFieldToRealTime(directoryEntry->createTime, &realTime);
        realTime.second += directoryEntry->createTimeTengthSec / 10;
        timeConvertRealTimeToTimestamp(&realTime, &timestamp);
        createTime = timestamp.second;
        
        __FAT32convertDateFieldToRealTime(directoryEntry->lastAccessDate, &realTime);
        timeConvertRealTimeToTimestamp(&realTime, &timestamp);
        lastAccessTime = timestamp.second;
        
        __FAT32convertDateFieldToRealTime(directoryEntry->lastModifyDate, &realTime);
        __FAT32convertTimeFieldToRealTime(directoryEntry->lastModifyTime, &realTime);
        timeConvertRealTimeToTimestamp(&realTime, &timestamp);
        lastModifyTime = timestamp.second;

        args = (FileSystemEntryDescriptorInitArgs) {
            .name           = nameBuffer,
            .type           = type,
            .dataRange      = RANGE(
                (clusterBegin * BPB->sectorPerCluster + info->dataBlockRange.begin) << superBlock->device->bytePerBlockShift,
                type == FILE_SYSTEM_ENTRY_TYPE_FILE ? directoryEntry->size : ((FAT32getClusterChainLength(info, clusterBegin) * BPB->sectorPerCluster) << superBlock->device->bytePerBlockShift)
            ),
            .parent         = directory->descriptor,
            .flags          = EMPTY_FLAGS,
            .createTime     = createTime,
            .lastAccessTime = lastAccessTime,
            .lastModifyTime = lastModifyTime
        };

        if (TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY)) {
            SET_FLAG_BACK(args.flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_READ_ONLY);
        }

        if (TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN)) {
            SET_FLAG_BACK(args.flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_HIDDEN);
        }

        initFileSystemEntryDescriptor(childDesc, &args);
    }


    if (entrySizePtr != NULL) {
        *entrySizePtr = entrySize;
    }

    return RESULT_CONTINUE;
}

static Size __FAT32getDirectoryEntrySize(FileSystemEntryDescriptor* descriptor);

static Result __FAT32descToDirectoryEntry(FAT32info* info, FileSystemEntryDescriptor* descriptor, void* buffer);

static Result __FAT32addChild(FileSystemEntry* directory, FileSystemEntryDescriptor* childToAdd) {
    Size directoryEntrySize = __FAT32getDirectoryEntrySize(childToAdd);
    void* directoryEntry = kMalloc(directoryEntrySize);
    FAT32info* info = (FAT32info*)directory->iNode->superBlock->specificInfo;
    if (directoryEntry == NULL || __FAT32descToDirectoryEntry(info, childToAdd, directoryEntry) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    rawFileSystemEntrySeek(directory, directory->descriptor->dataRange.length);

    if (rawFileSystemEntryWrite(directory, directoryEntry, directoryEntrySize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;

}

static Result __FAT32removeChild(FileSystemEntry* directory, FileSystemEntryIdentifier* childToRemove) {
    Size oldDirectoryEntrySize;
    if (directoryLookup(directory, NULL, &oldDirectoryEntrySize, childToRemove) != RESULT_SUCCESS) {
        return RESULT_FAIL;
    }

    Index64 oldPointer = directory->pointer;
    rawFileSystemEntrySeek(directory, directory->pointer + oldDirectoryEntrySize);

    Size followedDataSize = directory->descriptor->dataRange.length - directory->pointer;
    void* followedData = kMalloc(followedDataSize);
    if (followedData == NULL || rawFileSystemEntryRead(directory, followedData, followedDataSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    rawFileSystemEntrySeek(directory, directory->pointer + oldDirectoryEntrySize);
    if (rawFileSystemEntryWrite(directory, followedData, followedDataSize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __FAT32updateChild(FileSystemEntry* directory, FileSystemEntryIdentifier* oldChild, FileSystemEntryDescriptor* newChild) {
    Size oldDirectoryEntrySize;
    if (directoryLookup(directory, NULL, &oldDirectoryEntrySize, oldChild) != RESULT_SUCCESS) {
        return RESULT_FAIL;
    }

    Size directoryEntrySize = __FAT32getDirectoryEntrySize(newChild);
    void* directoryEntry = kMalloc(directoryEntrySize); //TODO: Bad memory management
    FAT32info* info = (FAT32info*)directory->iNode->superBlock->specificInfo;
    if (directoryEntry == NULL || __FAT32descToDirectoryEntry(info, newChild, directoryEntry) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Size followedDataSize = directory->descriptor->dataRange.length - directory->pointer - oldDirectoryEntrySize;
    void* followedData;
    if (oldDirectoryEntrySize != directoryEntrySize) {
        Index64 oldPointer = directory->pointer;
        rawFileSystemEntrySeek(directory, directory->pointer + oldDirectoryEntrySize);
        followedData = kMalloc(followedDataSize); //TODO: Bad memory management
        if (followedData == NULL || rawFileSystemEntryRead(directory, followedData, followedDataSize) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
        rawFileSystemEntrySeek(directory, oldPointer);
    }

    if (rawFileSystemEntryWrite(directory, directoryEntry, directoryEntrySize) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (oldDirectoryEntrySize != directoryEntrySize) {
        rawFileSystemEntrySeek(directory, directory->pointer + oldDirectoryEntrySize);
        if (rawFileSystemEntryWrite(directory, followedData, followedDataSize) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return RESULT_SUCCESS;
}

static Result __FAT32descToDirectoryEntry(FAT32info* info, FileSystemEntryDescriptor* descriptor, void* buffer) {
    ConstCstring name = descriptor->identifier.name;
    if (name == NULL) {
        return RESULT_FAIL;
    }

    Size nameLength = strlen(descriptor->identifier.name); 
    if (nameLength > __FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH) {
        return RESULT_FAIL;
    }

    for (int i = 0; i < nameLength; ++i) {
        if (name[i] == '+' || name[i] == ',' || name[i] == ';' || name[i] == '=' || name[i] == '[' || name[i] == ']') {
            return RESULT_FAIL;
        }
    }

    Uint8 longNameEntryNum = DIVIDE_ROUND_UP(nameLength + 1, __FAT32_LONG_NAME_ENTRY_LEN);
    __FAT32DirectoryEntry* directoryEntry = (__FAT32DirectoryEntry*)(buffer + longNameEntryNum * sizeof(__FAT32LongNameEntry));
    memset(directoryEntry->name, ' ', sizeof(directoryEntry->name));
    for (int i = 0; i < __FAT32_DIRECTORY_ENTRY_NAME_MAIN_LENGTH && name[i] != '\0' && name[i] != '.'; ++i) {
        char ch = name[i];
        if ('a' <= ch && ch <= 'z') {
            ch += ('A' - 'a');
        }

        directoryEntry->name[i] = ch;
    }

    for (int i = 0; i < __FAT32_DIRECTORY_ENTRY_NAME_EXT_LENGTH; ++i) {
        char ch = name[nameLength - i - 1];
        if ('a' <= ch && ch <= 'z') {   //TODO: Use character functions
            ch += ('A' - 'a');
        }

        directoryEntry->name[__FAT32_DIRECTORY_ENTRY_NAME_LENGTH - i - 1] = ch;
    }

    directoryEntry->attribute           = EMPTY_FLAGS;
    if (TEST_FLAGS(descriptor->flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_READ_ONLY)) {
        SET_FLAG_BACK(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY);
    }

    if (TEST_FLAGS(descriptor->flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_HIDDEN)) {
        SET_FLAG_BACK(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN);
    }

    SET_FLAG_BACK(
        directoryEntry->attribute, 
        descriptor->identifier.type == FILE_SYSTEM_ENTRY_TYPE_DIRECTOY ? __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY : __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_ARCHIVE
    );

    directoryEntry->reserved            = 0;

    RealTime realTime;
    Timestamp timestamp;
    timestamp.second = descriptor->createTime;
    timeConvertTimestampToRealTime(&timestamp, &realTime);
    directoryEntry->createTimeTengthSec = (realTime.second % 2) * 10;
    directoryEntry->createTime          = __FAT32convertRealTimeToTimeField(&realTime);
    directoryEntry->createDate          = __FAT32convertRealTimeToDateField(&realTime);

    timestamp.second = descriptor->lastAccessTime;
    timeConvertTimestampToRealTime(&timestamp, &realTime);
    directoryEntry->lastAccessDate      = __FAT32convertRealTimeToDateField(&realTime);

    FAT32BPB* BPB = info->BPB;
    Index32 beginCluster = (descriptor->dataRange.begin / BPB->bytePerSector - info->dataBlockRange.begin) / BPB->sectorPerCluster;
    directoryEntry->clusterBeginHigh    = EXTRACT_VAL(beginCluster, 32, 16, 32);

    timestamp.second = descriptor->lastModifyTime;
    timeConvertTimestampToRealTime(&timestamp, &realTime);
    directoryEntry->lastModifyTime      = __FAT32convertRealTimeToTimeField(&realTime);
    directoryEntry->lastModifyDate      = __FAT32convertRealTimeToDateField(&realTime);

    directoryEntry->clusterBeginLow     = EXTRACT_VAL(beginCluster, 32, 0, 16);
    directoryEntry->size                = descriptor->dataRange.length;

    Uint8 checkSum = 0;
    for (int i = 0; i < __FAT32_DIRECTORY_ENTRY_NAME_LENGTH; ++i) {
        checkSum = ((checkSum & 1) ? 0x80 : 0) + (checkSum >> 1) + directoryEntry->name[i];
    }

    for (int i = longNameEntryNum; i >= 1; --i) {
        __FAT32LongNameEntry* longNameEntry = ((__FAT32LongNameEntry*)buffer) + longNameEntryNum - i;
        longNameEntry->order = (i == longNameEntryNum ? (Uint8)VAL_XOR(longNameEntryNum, __FAT32_LONG_NAME_ENTRY_FLAG) : i);
        Uint8 offset = (i - 1) * __FAT32_LONG_NAME_ENTRY_LEN, partLen = umin8(__FAT32_LONG_NAME_ENTRY_LEN, nameLength  + 1 - offset);
        ConstCstring part = name + offset;

        int current = 0;    //Trailing 0 is included
        for (int j = 0; j < __FAT32_LONG_NAME_ENTRY_LEN1; ++j, ++current) {
            longNameEntry->doubleBytes1[j] = current < partLen ? (Uint16)part[current] : 0xFFFF;
        }

        for (int j = 0; j < __FAT32_LONG_NAME_ENTRY_LEN2; ++j, ++current) {
            longNameEntry->doubleBytes2[j] = current < partLen ? (Uint16)part[current] : 0xFFFF;
        }

        for (int j = 0; j < __FAT32_LONG_NAME_ENTRY_LEN3; ++j, ++current) {
            longNameEntry->doubleBytes3[j] = current < partLen ? (Uint16)part[current] : 0xFFFF;
        }

        longNameEntry->attribute    = __FAT32_LONG_NAME_ENTRY_ATTRIBUTE;
        longNameEntry->type         = 0;
        longNameEntry->checksum     = checkSum;
        longNameEntry->reserved     = 0;
    }

    return RESULT_SUCCESS;
}

static Size __FAT32getDirectoryEntrySize(FileSystemEntryDescriptor* descriptor) {
    Size nameLength = strlen(descriptor->identifier.name);
    return DIVIDE_ROUND_UP(nameLength, __FAT32_LONG_NAME_ENTRY_LEN) * sizeof(__FAT32LongNameEntry) + sizeof(__FAT32DirectoryEntry);
}

static void __FAT32convertDateFieldToRealTime(Uint16 date, RealTime* realTime) {
    realTime->year  = 1980 + EXTRACT_VAL(date, 16, 9, 16);
    realTime->month = EXTRACT_VAL(date, 16, 5, 9);
    realTime->day   = EXTRACT_VAL(date, 16, 0, 5);
}

static void __FAT32convertTimeFieldToRealTime(Uint16 time, RealTime* realTime) {
    realTime->hour      = EXTRACT_VAL(time, 16, 11, 15);
    realTime->minute    = EXTRACT_VAL(time, 16, 5, 11);
    realTime->second    = EXTRACT_VAL(time, 16, 0, 5) << 1;
}

static Uint16 __FAT32convertRealTimeToDateField(RealTime* realTime) {
    return VAL_LEFT_SHIFT(umax16((Uint16)realTime->year, 1980) - 1980, 9) | VAL_LEFT_SHIFT((Uint16)realTime->month, 5) | ((Uint16)realTime->day);
}

static Uint16 __FAT32convertRealTimeToTimeField(RealTime* realTime) {
    return VAL_LEFT_SHIFT((Uint16)realTime->hour, 11) | VAL_LEFT_SHIFT((Uint16)realTime->minute, 5) | ((Uint16)realTime->second >> 1);
}