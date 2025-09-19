#include<fs/fat32/directoryEntry.h>

#include<fs/vnode.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<structs/string.h>
#include<time/time.h>
#include<algorithms.h>
#include<cstring.h>
#include<debug.h>
#include<error.h>

static void __fat32_directoryEntry_parseLongName(FAT32LongNameEntry* entries, Size longNameEntryNum, String* nameOut);

static void __fat32_directoryEntry_parseShortName(FAT32DirectoryEntry* entry, String* nameOut);

static void __fat32_directoryEntry_parseEntry(FAT32DirectoryEntry* entry, Uint8* attributeOut, FSnodeAttribute* fsnodeAttributeOut, Index32* firstClusterOut, Size* sizeOut);

static void __fat32_directoryEntry_convertDateFieldToRealTime(Uint16 date, RealTime* realTime);

static void __fat32_directoryEntry_convertTimeFieldToRealTime(Uint16 time, RealTime* realTime);

static Uint16 __fat32_directoryEntry_convertRealTimeToDateField(RealTime* realTime);

static Uint16 __fat32_directoryEntry_convertRealTimeToTimeField(RealTime* realTime);

void fat32_directoryEntry_parse(FAT32UnknownTypeEntry* entriesBegin, String* nameOut, Flags8* attributeOut, FSnodeAttribute* fsnodeAttributeOut, Index32* firstClusterOut, Size* sizeOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(nameOut));
    DEBUG_ASSERT_SILENT(!fat32_directoryEntry_isEnd(entriesBegin));
    
    Size length = fat32_directoryEntry_getEntriesLength(entriesBegin);
    if (length > sizeof(FAT32DirectoryEntry)) {
        Size longNameEntryNum = (length - sizeof(FAT32DirectoryEntry)) / sizeof(FAT32LongNameEntry);
        __fat32_directoryEntry_parseLongName((FAT32LongNameEntry*)entriesBegin, longNameEntryNum, nameOut);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        __fat32_directoryEntry_parseShortName((FAT32DirectoryEntry*)entriesBegin, nameOut);
        ERROR_GOTO_IF_ERROR(0);
    }

    __fat32_directoryEntry_parseEntry((FAT32DirectoryEntry*)((void*)entriesBegin + length - sizeof(FAT32DirectoryEntry)), attributeOut, fsnodeAttributeOut, firstClusterOut, sizeOut);

    return;
    ERROR_FINAL_BEGIN(0);
}

Size fat32_directoryEntry_initEntries(FAT32UnknownTypeEntry* entriesBegin, ConstCstring name, fsEntryType type, FSnodeAttribute* attribute, Index32 firstCluster, Size size) {
    Size nameLength = cstring_strlen(name);
    if (nameLength > FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    for (int i = 0; i < nameLength; ++i) {
        if (name[i] == '+' || name[i] == ',' || name[i] == ';' || name[i] == '=' || name[i] == '[' || name[i] == ']') {
            ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
        }
    }

    Uint8 longNameEntryNum = DIVIDE_ROUND_UP(nameLength + 1, FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN);
    FAT32DirectoryEntry* directoryEntry = (FAT32DirectoryEntry*)((void*)entriesBegin + longNameEntryNum * sizeof(FAT32LongNameEntry));
    memory_memset(directoryEntry->name, ' ', sizeof(directoryEntry->name));
    for (int i = 0; i < FAT32_DIRECTORY_ENTRY_NAME_MAIN_LENGTH && name[i] != '\0' && name[i] != '.'; ++i) {
        char ch = name[i];
        if ('a' <= ch && ch <= 'z') {
            ch += ('A' - 'a');
        }

        directoryEntry->name[i] = ch;
    }

    for (int i = 0; i < FAT32_DIRECTORY_ENTRY_NAME_EXT_LENGTH; ++i) {
        char ch = name[nameLength - i - 1];
        if ('a' <= ch && ch <= 'z') {   //TODO: Use character functions
            ch += ('A' - 'a');
        }

        directoryEntry->name[FAT32_DIRECTORY_ENTRY_NAME_LENGTH - i - 1] = ch;
    }

    directoryEntry->attribute           = EMPTY_FLAGS;
    SET_FLAG_BACK(
        directoryEntry->attribute, 
        type == FS_ENTRY_TYPE_DIRECTORY ? FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY : FAT32_DIRECTORY_ENTRY_ATTRIBUTE_ARCHIVE
    );

    directoryEntry->reserved            = 0;

    RealTime realTime;
    Timestamp timestamp;
    timestamp.second = attribute->createTime;
    time_convertTimestampToRealTime(&timestamp, &realTime);
    directoryEntry->createTimeTengthSec = (realTime.second % 2) * 10;
    directoryEntry->createTime          = __fat32_directoryEntry_convertRealTimeToTimeField(&realTime);
    directoryEntry->createDate          = __fat32_directoryEntry_convertRealTimeToDateField(&realTime);

    timestamp.second = attribute->lastAccessTime;
    time_convertTimestampToRealTime(&timestamp, &realTime);
    directoryEntry->lastAccessDate      = __fat32_directoryEntry_convertRealTimeToDateField(&realTime);

    directoryEntry->clusterBeginHigh    = EXTRACT_VAL(firstCluster, 32, 16, 32);

    timestamp.second = attribute->lastModifyTime;
    time_convertTimestampToRealTime(&timestamp, &realTime);
    directoryEntry->lastModifyTime      = __fat32_directoryEntry_convertRealTimeToTimeField(&realTime);
    directoryEntry->lastModifyDate      = __fat32_directoryEntry_convertRealTimeToDateField(&realTime);

    directoryEntry->clusterBeginLow     = EXTRACT_VAL(firstCluster, 32, 0, 16);
    directoryEntry->size                = size;

    Uint8 checkSum = 0;
    for (int i = 0; i < FAT32_DIRECTORY_ENTRY_NAME_LENGTH; ++i) {
        checkSum = ((checkSum & 1) ? 0x80 : 0) + (checkSum >> 1) + directoryEntry->name[i];
    }

    for (int i = longNameEntryNum; i >= 1; --i) {
        FAT32LongNameEntry* longNameEntry = (FAT32LongNameEntry*)((void*)entriesBegin + (longNameEntryNum - i) * sizeof(FAT32LongNameEntry));
        longNameEntry->order = (i == longNameEntryNum ? (Uint8)VAL_XOR(longNameEntryNum, FAT32_DIRECTORY_ENTRY_LONG_NAME_FLAG) : i);
        Uint8 offset = (i - 1) * FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN, partLen = algorithms_umin8(FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN, nameLength  + 1 - offset);
        ConstCstring part = name + offset;

        int current = 0;    //Trailing 0 is included
        for (int j = 0; j < FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN1; ++j, ++current) {
            longNameEntry->doubleBytes1[j] = current < partLen ? (Uint16)part[current] : 0xFFFF;
        }

        for (int j = 0; j < FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN2; ++j, ++current) {
            longNameEntry->doubleBytes2[j] = current < partLen ? (Uint16)part[current] : 0xFFFF;
        }

        for (int j = 0; j < FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN3; ++j, ++current) {
            longNameEntry->doubleBytes3[j] = current < partLen ? (Uint16)part[current] : 0xFFFF;
        }

        longNameEntry->attribute    = FAT32_DIRECTORY_ENTRY_LONG_NAME_ATTRIBUTE;
        longNameEntry->type         = 0;
        longNameEntry->checksum     = checkSum;
        longNameEntry->reserved     = 0;
    }

    return longNameEntryNum * sizeof(FAT32LongNameEntry) + sizeof(FAT32DirectoryEntry);
    ERROR_FINAL_BEGIN(0);
    return 0;
}

void __fat32_directoryEntry_parseLongName(FAT32LongNameEntry* entries, Size longNameEntryNum, String* nameOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(nameOut));
    DEBUG_ASSERT_SILENT(TEST_FLAGS(entries[0].order, FAT32_DIRECTORY_ENTRY_LONG_NAME_FLAG));
    string_clear(nameOut);

    char namePartBuffer[FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN + 1];
    Size namePartLength = 0;
    for (int i = longNameEntryNum - 1; i >= 0; --i) {
        FAT32LongNameEntry* longNameEntry = &entries[i];

        bool notEnd = true;
        for (int i = 0; i < FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN1 && (notEnd &= (longNameEntry->doubleBytes1[i] != 0)) && namePartLength < FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN; ++i) {
            namePartBuffer[namePartLength++] = (char)longNameEntry->doubleBytes1[i];
        }

        for (int i = 0; i < FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN2 && (notEnd &= (longNameEntry->doubleBytes2[i] != 0)) && namePartLength < FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN; ++i) {
            namePartBuffer[namePartLength++] = (char)longNameEntry->doubleBytes2[i];
        }

        for (int i = 0; i < FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN3 && (notEnd &= (longNameEntry->doubleBytes3[i] != 0)) && namePartLength < FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN; ++i) {
            namePartBuffer[namePartLength++] = (char)longNameEntry->doubleBytes3[i];
        }

        if (namePartLength == FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN && notEnd) {
            ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
        }

        namePartBuffer[namePartLength] = '\0';
        string_cconcat(nameOut, nameOut, namePartBuffer);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void __fat32_directoryEntry_parseShortName(FAT32DirectoryEntry* entry, String* nameOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(nameOut));
    string_clear(nameOut);

    char nameBuffer[FAT32_DIRECTORY_ENTRY_NAME_LENGTH + 2];
    Size nameLength = 0;
    for (int i = 0; i < 8 && entry->name[i] != ' '; ++i) {
        nameBuffer[nameLength++] = entry->name[i];
    }

    if (entry->name[8] != ' ') {    //Has extension
        nameBuffer[nameLength++] = '.';
        for (int i = 8; i < 11 && entry->name[i] != ' '; ++i) {
            nameBuffer[nameLength++] = entry->name[i];
        }
    }
    nameBuffer[nameLength] = '\0';

    string_cconcat(nameOut, nameOut, nameBuffer);   //Error passthrough
    return;
}

void __fat32_directoryEntry_parseEntry(FAT32DirectoryEntry* entry, Uint8* attributeOut, FSnodeAttribute* fsnodeAttributeOut, Index32* firstClusterOut, Size* sizeOut) {
    *attributeOut = entry->attribute;
    
    Uint64 createTime = 0, lastAccessTime = 0, lastModifyTime = 0;
    RealTime realTime;
    Timestamp timestamp;
    memory_memset(&realTime, 0, sizeof(RealTime));
    __fat32_directoryEntry_convertDateFieldToRealTime(entry->createDate, &realTime);
    __fat32_directoryEntry_convertTimeFieldToRealTime(entry->createTime, &realTime);
    realTime.second += entry->createTimeTengthSec / 10;
    time_convertRealTimeToTimestamp(&realTime, &timestamp);
    createTime = timestamp.second;
    
    memory_memset(&realTime, 0, sizeof(RealTime));
    __fat32_directoryEntry_convertDateFieldToRealTime(entry->lastAccessDate, &realTime);
    time_convertRealTimeToTimestamp(&realTime, &timestamp);
    lastAccessTime = timestamp.second;
    
    memory_memset(&realTime, 0, sizeof(RealTime));
    __fat32_directoryEntry_convertDateFieldToRealTime(entry->lastModifyDate, &realTime);
    __fat32_directoryEntry_convertTimeFieldToRealTime(entry->lastModifyTime, &realTime);
    time_convertRealTimeToTimestamp(&realTime, &timestamp);
    lastModifyTime = timestamp.second;
    
    *fsnodeAttributeOut = (FSnodeAttribute) {
        .uid = 0,
        .gid = 0,
        .createTime = createTime,
        .lastAccessTime = lastAccessTime,
        .lastModifyTime = lastModifyTime
    };

    *firstClusterOut = ((Uint32)entry->clusterBeginHigh << 16) | entry->clusterBeginLow;
    *sizeOut = entry->size;
}

static void __fat32_directoryEntry_convertDateFieldToRealTime(Uint16 date, RealTime* realTime) {
    realTime->year  = 1980 + EXTRACT_VAL(date, 16, 9, 16);
    realTime->month = EXTRACT_VAL(date, 16, 5, 9);
    realTime->day   = EXTRACT_VAL(date, 16, 0, 5);
}

static void __fat32_directoryEntry_convertTimeFieldToRealTime(Uint16 time, RealTime* realTime) {
    realTime->hour      = EXTRACT_VAL(time, 16, 11, 15);
    realTime->minute    = EXTRACT_VAL(time, 16, 5, 11);
    realTime->second    = EXTRACT_VAL(time, 16, 0, 5) << 1;
}

static Uint16 __fat32_directoryEntry_convertRealTimeToDateField(RealTime* realTime) {
    return VAL_LEFT_SHIFT(algorithms_umax16((Uint16)realTime->year, 1980) - 1980, 9) | VAL_LEFT_SHIFT((Uint16)realTime->month, 5) | ((Uint16)realTime->day);
}

static Uint16 __fat32_directoryEntry_convertRealTimeToTimeField(RealTime* realTime) {
    return VAL_LEFT_SHIFT((Uint16)realTime->hour, 11) | VAL_LEFT_SHIFT((Uint16)realTime->minute, 5) | ((Uint16)realTime->second >> 1);
}