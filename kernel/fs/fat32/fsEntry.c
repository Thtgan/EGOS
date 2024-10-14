#include<fs/fat32/fsEntry.h>

#include<algorithms.h>
#include<cstring.h>
#include<fs/fat32/cluster.h>
#include<fs/fat32/fat32.h>
#include<fs/fat32/inode.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsutil.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/string.h>
#include<time/time.h>

typedef struct __FAT32DirectoryEntry {
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

typedef struct __FAT32LongNameEntry {
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

static Result __fat32_fsEntry_readEntry(fsEntry* directory, fsEntryDesc* childDesc, Size* entrySizePtr);

static Result __fat32_fsEntry_addEntry(fsEntry* directory, fsEntryDesc* childToAdd);

static Result __fat32_fsEntry_removeEntry(fsEntry* directory, fsEntryIdentifier* childToRemove);

static Result __fat32_fsEntry_updateEntry(fsEntry* directory, fsEntryIdentifier* oldChild, fsEntryDesc* newChild);

static fsEntryOperations _fat32_fsEntryOperations = {
    .seek           = fsEntry_genericSeek,
    .read           = fsEntry_genericRead,
    .write          = fsEntry_genericWrite,
    .resize         = fsEntry_genericResize
};

//TODO: These functions seems wrong...
static fsEntryDirOperations _fat32_fsEntryDirOperations = {
    .readEntry      = __fat32_fsEntry_readEntry,
    .addEntry       = __fat32_fsEntry_addEntry,
    .removeEntry    = __fat32_fsEntry_removeEntry,
    .updateEntry    = __fat32_fsEntry_updateEntry
};

Result fat32_fsEntry_open(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags) {
    if (fsEntry_genericOpen(superBlock, entry, desc, flags) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    entry->operations = &_fat32_fsEntryOperations;
    if (entry->desc->type == FS_ENTRY_TYPE_DIRECTORY) {
        entry->dirOperations = &_fat32_fsEntryDirOperations;
        
        Size directoryDataSize = 0, entrySize;
        fsEntry_rawSeek(entry, 0);

        Result res = RESULT_ERROR;
        while((res = fsEntry_rawDirReadEntry(entry, NULL, &entrySize)) != RESULT_ERROR) {
            if (res == RESULT_SUCCESS) {
                break;
            }

            directoryDataSize += entrySize;
            fsEntry_rawSeek(entry, entry->pointer + entrySize);
        }

        if (res != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        entry->desc->dataRange.length = directoryDataSize;
    }

    return RESULT_SUCCESS;
}

Result fat32_fsEntry_create(SuperBlock* superBlock, fsEntryDesc* descOut, ConstCstring name, ConstCstring parentPath, fsEntryType type, DeviceID deviceID, Flags16 flags) {
    FAT32info* info = (FAT32info*)superBlock->specificInfo;
    FAT32BPB* BPB = info->BPB;

    Index32 newCluster = fat32_allocateClusterChain(info, 1);
    if (newCluster == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    Device* device = &superBlock->blockDevice->device;
    Timestamp timestamp;
    time_getTimestamp(&timestamp);
    fsEntryDescInitArgs args = {
        .name           = name,
        .parentPath     = parentPath,
        .type           = type,
        .flags          = flags,
        .createTime     = timestamp.second,
        .lastAccessTime = timestamp.second,
        .lastModifyTime = timestamp.second
    };

    if (type == FS_ENTRY_TYPE_DEVICE) {
        args.device     = deviceID;
    } else {
        args.dataRange  = RANGE((newCluster * BPB->sectorPerCluster + info->dataBlockRange.begin) * POWER_2(device->granularity), 0);
    }

    fsEntryDesc_initStruct(descOut, &args);

    return RESULT_SUCCESS;
}

static Result __fat32_fsEntry_doReadEntry(fsEntry* directory, fsEntryDesc* childDesc, Size* entrySizePtr, char** bufferPtr, String* directoryPath);

static Result __fat32_fsEntry_readEntry(fsEntry* directory, fsEntryDesc* childDesc, Size* entrySizePtr) {
    char* buffer;
    String directoryPath;
    Result res = __fat32_fsEntry_doReadEntry(directory, childDesc, entrySizePtr, &buffer, &directoryPath);

    if (STRING_IS_AVAILABLE(&directoryPath)) {
        string_clearStruct(&directoryPath);
    }

    if (buffer != NULL) {
        memory_free(buffer);
    }

    return res;
}

static void __fat32_fsEntry_convertDateFieldToRealTime(Uint16 date, RealTime* realTime);

static void __fat32_fsEntry_convertTimeFieldToRealTime(Uint16 time, RealTime* realTime);

static Uint16 __fat32_fsEntry_convertRealTimeToDateField(RealTime* realTime);

static Uint16 __fat32_fsEntry_convertRealTimeToTimeField(RealTime* realTime);

static Result __fat32_fsEntry_doReadEntry(fsEntry* directory, fsEntryDesc* childDesc, Size* entrySizePtr, char** bufferPtr, String* directoryPath) {
    if ((*bufferPtr = memory_allocate(BLOCK_DEVICE_DEFAULT_BLOCK_SIZE)) == NULL || fsEntry_rawRead(directory, *bufferPtr, sizeof(__FAT32DirectoryEntry)) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }
    char* buffer = *bufferPtr;

    if (string_concat(directoryPath, &directory->desc->identifier.parentPath, &directory->desc->identifier.name) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    fsEntryDescInitArgs args;
    if (buffer[0] == 0x00 || buffer[0] == 0xE5) {
        if (childDesc != NULL) {
            args = (fsEntryDescInitArgs) {
                .name       = "",
                .parentPath = directoryPath->data,
                .type       = FS_ENTRY_TYPE_DUMMY,
                .dataRange  = RANGE(FS_ENTRY_INVALID_POSITION, FS_ENTRY_INVALID_SIZE),
                .flags      = EMPTY_FLAGS
            };

            fsEntryDesc_initStruct(childDesc, &args);
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
            return RESULT_ERROR;
        }

        longNameEntryNum = (Uint8)VAL_XOR(longNameEntry->order, __FAT32_LONG_NAME_ENTRY_FLAG);
    }

    Size entrySize = longNameEntryNum * sizeof(__FAT32LongNameEntry) + sizeof(__FAT32DirectoryEntry);

    if (childDesc != NULL) {
        if (fsEntry_rawRead(directory, buffer, entrySize) != RESULT_SUCCESS) {
            return RESULT_ERROR;
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
                    return RESULT_ERROR;
                }
            }
        }
        nameBuffer[nameLength]  = '\0';

        SuperBlock* superBlock  = directory->iNode->superBlock;
        FAT32info* info         = (FAT32info*)superBlock->specificInfo;
        FAT32BPB* BPB           = info->BPB;

        Index32 clusterBegin    = ((Uint32)directoryEntry->clusterBeginHigh << 16) | directoryEntry->clusterBeginLow;
        fsEntryType type        = TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY) ? FS_ENTRY_TYPE_DIRECTORY : FS_ENTRY_TYPE_FILE;

        Uint64 createTime, lastAccessTime, lastModifyTime;
        RealTime realTime;
        Timestamp timestamp;
        __fat32_fsEntry_convertDateFieldToRealTime(directoryEntry->createDate, &realTime);
        __fat32_fsEntry_convertTimeFieldToRealTime(directoryEntry->createTime, &realTime);
        realTime.second += directoryEntry->createTimeTengthSec / 10;
        time_convertRealTimeToTimestamp(&realTime, &timestamp);
        createTime = timestamp.second;
        
        __fat32_fsEntry_convertDateFieldToRealTime(directoryEntry->lastAccessDate, &realTime);
        time_convertRealTimeToTimestamp(&realTime, &timestamp);
        lastAccessTime = timestamp.second;
        
        __fat32_fsEntry_convertDateFieldToRealTime(directoryEntry->lastModifyDate, &realTime);
        __fat32_fsEntry_convertTimeFieldToRealTime(directoryEntry->lastModifyTime, &realTime);
        time_convertRealTimeToTimestamp(&realTime, &timestamp);
        lastModifyTime = timestamp.second;

        Device* superBlockDevice = &superBlock->blockDevice->device;
        args = (fsEntryDescInitArgs) {
            .name           = nameBuffer,
            .parentPath     = directoryPath->data,
            .type           = type,
            .dataRange      = RANGE(
                (clusterBegin * BPB->sectorPerCluster + info->dataBlockRange.begin) << superBlockDevice->granularity,
                type == FS_ENTRY_TYPE_FILE ? directoryEntry->size : ((fat32_getClusterChainLength(info, clusterBegin) * BPB->sectorPerCluster) << superBlockDevice->granularity)
            ),
            .flags          = EMPTY_FLAGS,
            .createTime     = createTime,
            .lastAccessTime = lastAccessTime,
            .lastModifyTime = lastModifyTime
        };

        if (TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY)) {
            SET_FLAG_BACK(args.flags, FS_ENTRY_DESC_FLAGS_READ_ONLY);
        }

        if (TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN)) {
            SET_FLAG_BACK(args.flags, FS_ENTRY_DESC_FLAGS_HIDDEN);
        }

        fsEntryDesc_initStruct(childDesc, &args);
    }


    if (entrySizePtr != NULL) {
        *entrySizePtr = entrySize;
    }

    return RESULT_CONTINUE;
}

static Size __fat32_fsEntry_getDirectoryEntrySize(fsEntryDesc* desc);

static Result __fat32_fsEntry_descToDirectoryEntry(FAT32info* info, fsEntryDesc* desc, void* buffer);

static Result __fat32_fsEntry_addEntry(fsEntry* directory, fsEntryDesc* childToAdd) { //TODO: Not considering dummy at tail
    Size directoryEntrySize = __fat32_fsEntry_getDirectoryEntrySize(childToAdd);
    void* directoryEntry = memory_allocate(directoryEntrySize);
    FAT32info* info = (FAT32info*)directory->iNode->superBlock->specificInfo;
    if (directoryEntry == NULL || __fat32_fsEntry_descToDirectoryEntry(info, childToAdd, directoryEntry) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    fsEntry_rawSeek(directory, directory->desc->dataRange.length);

    if (fsEntry_rawWrite(directory, directoryEntry, directoryEntrySize) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    memory_free(directoryEntry);

    return RESULT_SUCCESS;
}

static Result __fat32_fsEntry_removeEntry(fsEntry* directory, fsEntryIdentifier* childToRemove) {
    Size oldDirectoryEntrySize;
    if (fsutil_lookupEntryDesc(directory, childToRemove->name.data, childToRemove->isDirectory, NULL, &oldDirectoryEntrySize) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    Index64 oldPointer = directory->pointer;
    fsEntry_rawSeek(directory, directory->pointer + oldDirectoryEntrySize);

    Size followedDataSize = directory->desc->dataRange.length - directory->pointer;
    void* followedData = memory_allocate(followedDataSize);
    if (followedData == NULL || fsEntry_rawRead(directory, followedData, followedDataSize) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    fsEntry_rawSeek(directory, oldPointer);
    if (fsEntry_rawWrite(directory, followedData, followedDataSize) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    memory_free(followedData);

    return RESULT_SUCCESS;
}

static Result __fat32_fsEntry_updateEntry(fsEntry* directory, fsEntryIdentifier* oldChild, fsEntryDesc* newChild) {
    Size oldDirectoryEntrySize;
    if (fsutil_lookupEntryDesc(directory, oldChild->name.data, oldChild->isDirectory, NULL, &oldDirectoryEntrySize) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    Size directoryEntrySize = __fat32_fsEntry_getDirectoryEntrySize(newChild);
    void* directoryEntry = memory_allocate(directoryEntrySize); //TODO: Bad memory management
    FAT32info* info = (FAT32info*)directory->iNode->superBlock->specificInfo;
    if (directoryEntry == NULL || __fat32_fsEntry_descToDirectoryEntry(info, newChild, directoryEntry) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    Size followedDataSize = directory->desc->dataRange.length - directory->pointer - oldDirectoryEntrySize;
    void* followedData;
    if (oldDirectoryEntrySize != directoryEntrySize) {
        Index64 oldPointer = directory->pointer;
        fsEntry_rawSeek(directory, directory->pointer + oldDirectoryEntrySize);
        followedData = memory_allocate(followedDataSize); //TODO: Bad memory management
        if (followedData == NULL || fsEntry_rawRead(directory, followedData, followedDataSize) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }
        fsEntry_rawSeek(directory, oldPointer);
    }

    if (fsEntry_rawWrite(directory, directoryEntry, directoryEntrySize) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    if (oldDirectoryEntrySize != directoryEntrySize) {
        fsEntry_rawSeek(directory, directory->pointer + oldDirectoryEntrySize);
        if (fsEntry_rawWrite(directory, followedData, followedDataSize) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        memory_free(followedData);
    }

    memory_free(directoryEntry);

    return RESULT_SUCCESS;
}

static Result __fat32_fsEntry_descToDirectoryEntry(FAT32info* info, fsEntryDesc* desc, void* buffer) {
    ConstCstring name = desc->identifier.name.data;
    Size nameLength = desc->identifier.name.length; 
    if (nameLength > __FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH) {
        return RESULT_ERROR;
    }

    for (int i = 0; i < nameLength; ++i) {
        if (name[i] == '+' || name[i] == ',' || name[i] == ';' || name[i] == '=' || name[i] == '[' || name[i] == ']') {
            return RESULT_ERROR;
        }
    }

    Uint8 longNameEntryNum = DIVIDE_ROUND_UP(nameLength + 1, __FAT32_LONG_NAME_ENTRY_LEN);
    __FAT32DirectoryEntry* directoryEntry = (__FAT32DirectoryEntry*)(buffer + longNameEntryNum * sizeof(__FAT32LongNameEntry));
    memory_memset(directoryEntry->name, ' ', sizeof(directoryEntry->name));
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
    if (TEST_FLAGS(desc->flags, FS_ENTRY_DESC_FLAGS_READ_ONLY)) {
        SET_FLAG_BACK(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY);
    }

    if (TEST_FLAGS(desc->flags, FS_ENTRY_DESC_FLAGS_HIDDEN)) {
        SET_FLAG_BACK(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN);
    }

    SET_FLAG_BACK(
        directoryEntry->attribute, 
        desc->type == FS_ENTRY_TYPE_DIRECTORY ? __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY : __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_ARCHIVE
    );

    directoryEntry->reserved            = 0;

    RealTime realTime;
    Timestamp timestamp;
    timestamp.second = desc->createTime;
    time_convertTimestampToRealTime(&timestamp, &realTime);
    directoryEntry->createTimeTengthSec = (realTime.second % 2) * 10;
    directoryEntry->createTime          = __fat32_fsEntry_convertRealTimeToTimeField(&realTime);
    directoryEntry->createDate          = __fat32_fsEntry_convertRealTimeToDateField(&realTime);

    timestamp.second = desc->lastAccessTime;
    time_convertTimestampToRealTime(&timestamp, &realTime);
    directoryEntry->lastAccessDate      = __fat32_fsEntry_convertRealTimeToDateField(&realTime);

    FAT32BPB* BPB = info->BPB;
    Index32 beginCluster = (desc->dataRange.begin / BPB->bytePerSector - info->dataBlockRange.begin) / BPB->sectorPerCluster;
    directoryEntry->clusterBeginHigh    = EXTRACT_VAL(beginCluster, 32, 16, 32);

    timestamp.second = desc->lastModifyTime;
    time_convertTimestampToRealTime(&timestamp, &realTime);
    directoryEntry->lastModifyTime      = __fat32_fsEntry_convertRealTimeToTimeField(&realTime);
    directoryEntry->lastModifyDate      = __fat32_fsEntry_convertRealTimeToDateField(&realTime);

    directoryEntry->clusterBeginLow     = EXTRACT_VAL(beginCluster, 32, 0, 16);
    directoryEntry->size                = desc->dataRange.length;

    Uint8 checkSum = 0;
    for (int i = 0; i < __FAT32_DIRECTORY_ENTRY_NAME_LENGTH; ++i) {
        checkSum = ((checkSum & 1) ? 0x80 : 0) + (checkSum >> 1) + directoryEntry->name[i];
    }

    for (int i = longNameEntryNum; i >= 1; --i) {
        __FAT32LongNameEntry* longNameEntry = ((__FAT32LongNameEntry*)buffer) + longNameEntryNum - i;
        longNameEntry->order = (i == longNameEntryNum ? (Uint8)VAL_XOR(longNameEntryNum, __FAT32_LONG_NAME_ENTRY_FLAG) : i);
        Uint8 offset = (i - 1) * __FAT32_LONG_NAME_ENTRY_LEN, partLen = algorithms_umin8(__FAT32_LONG_NAME_ENTRY_LEN, nameLength  + 1 - offset);
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

static Size __fat32_fsEntry_getDirectoryEntrySize(fsEntryDesc* desc) {
    return DIVIDE_ROUND_UP(desc->identifier.name.length, __FAT32_LONG_NAME_ENTRY_LEN) * sizeof(__FAT32LongNameEntry) + sizeof(__FAT32DirectoryEntry);
}

static void __fat32_fsEntry_convertDateFieldToRealTime(Uint16 date, RealTime* realTime) {
    realTime->year  = 1980 + EXTRACT_VAL(date, 16, 9, 16);
    realTime->month = EXTRACT_VAL(date, 16, 5, 9);
    realTime->day   = EXTRACT_VAL(date, 16, 0, 5);
}

static void __fat32_fsEntry_convertTimeFieldToRealTime(Uint16 time, RealTime* realTime) {
    realTime->hour      = EXTRACT_VAL(time, 16, 11, 15);
    realTime->minute    = EXTRACT_VAL(time, 16, 5, 11);
    realTime->second    = EXTRACT_VAL(time, 16, 0, 5) << 1;
}

static Uint16 __fat32_fsEntry_convertRealTimeToDateField(RealTime* realTime) {
    return VAL_LEFT_SHIFT(algorithms_umax16((Uint16)realTime->year, 1980) - 1980, 9) | VAL_LEFT_SHIFT((Uint16)realTime->month, 5) | ((Uint16)realTime->day);
}

static Uint16 __fat32_fsEntry_convertRealTimeToTimeField(RealTime* realTime) {
    return VAL_LEFT_SHIFT((Uint16)realTime->hour, 11) | VAL_LEFT_SHIFT((Uint16)realTime->minute, 5) | ((Uint16)realTime->second >> 1);
}