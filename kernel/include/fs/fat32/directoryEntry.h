#if !defined(__FS_FAT32_DIRECTORYENTRY_H)
#define __FS_FAT32_DIRECTORYENTRY_H

typedef struct FAT32DirectoryEntry FAT32DirectoryEntry;
typedef struct FAT32LongNameEntry FAT32LongNameEntry;
typedef struct FAT32UnknownTypeEntry FAT32UnknownTypeEntry;

#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/vnode.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<structs/string.h>
#include<debug.h>

typedef struct FAT32DirectoryEntry {
#define FAT32_DIRECTORY_ENTRY_NAME_MAIN_LENGTH      8
#define FAT32_DIRECTORY_ENTRY_NAME_EXT_LENGTH       3
#define FAT32_DIRECTORY_ENTRY_NAME_LENGTH           (FAT32_DIRECTORY_ENTRY_NAME_MAIN_LENGTH + FAT32_DIRECTORY_ENTRY_NAME_EXT_LENGTH)
    char    name[FAT32_DIRECTORY_ENTRY_NAME_LENGTH];
    Uint8   attribute;
#define FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY   FLAG8(0)
#define FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN      FLAG8(1)
#define FAT32_DIRECTORY_ENTRY_ATTRIBUTE_SYSTEM      FLAG8(2)
#define FAT32_DIRECTORY_ENTRY_ATTRIBUTE_VOLUME_ID   FLAG8(3)
#define FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY   FLAG8(4)
#define FAT32_DIRECTORY_ENTRY_ATTRIBUTE_ARCHIVE     FLAG8(5)
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
} __attribute__((packed)) FAT32DirectoryEntry;

#define FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN1        5
#define FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN2        6
#define FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN3        2
#define FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN         (FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN1 + FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN2 + FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN3)
#define FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH   255

typedef struct FAT32LongNameEntry {
    Uint8   order;
#define FAT32_DIRECTORY_ENTRY_LONG_NAME_FLAG        FLAG8(6)
    Uint16  doubleBytes1[FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN1];
    Uint8   attribute;
#define FAT32_DIRECTORY_ENTRY_LONG_NAME_ATTRIBUTE   (FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY | FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN | FAT32_DIRECTORY_ENTRY_ATTRIBUTE_SYSTEM | FAT32_DIRECTORY_ENTRY_ATTRIBUTE_VOLUME_ID)
    Uint8   type;
    Uint8   checksum;
    Uint16  doubleBytes2[FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN2];
    Uint16  reserved;
    Uint16  doubleBytes3[FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN3];
} __attribute__((packed)) FAT32LongNameEntry;

DEBUG_ASSERT_COMPILE(sizeof(FAT32DirectoryEntry) == sizeof(FAT32LongNameEntry));

#define FAT32_DIRECTORY_ENTRY_MAX_ENTRIES_SIZE      (DIVIDE_ROUND_UP(FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH, FAT32_DIRECTORY_ENTRY_LONG_NAME_LEN) * sizeof(FAT32LongNameEntry) + sizeof(FAT32DirectoryEntry))

typedef struct FAT32UnknownTypeEntry {
    Uint8 padding1[11];
    Uint8 attribute;
    Uint8 padding2[20];
} __attribute__((packed)) FAT32UnknownTypeEntry;

DEBUG_ASSERT_COMPILE(sizeof(FAT32DirectoryEntry) == sizeof(FAT32UnknownTypeEntry));

static inline bool fat32_directoryEntry_isUsingLongNameEntry(FAT32UnknownTypeEntry* entry) {
    return entry->attribute == FAT32_DIRECTORY_ENTRY_LONG_NAME_ATTRIBUTE;
}

static inline Size fat32_directoryEntry_getEntriesLength(FAT32UnknownTypeEntry* entry) {
    if (fat32_directoryEntry_isUsingLongNameEntry(entry)) {
        FAT32LongNameEntry* longName = (FAT32LongNameEntry*)entry;
        DEBUG_ASSERT_SILENT(TEST_FLAGS(longName->order, FAT32_DIRECTORY_ENTRY_LONG_NAME_FLAG));
        return VAL_XOR(longName->order, FAT32_DIRECTORY_ENTRY_LONG_NAME_FLAG) * sizeof(FAT32LongNameEntry) + sizeof(FAT32DirectoryEntry);
    }

    return sizeof(FAT32DirectoryEntry);
}

static inline bool fat32_directoryEntry_isEnd(FAT32UnknownTypeEntry* entry) {
    Uint8 firstByte = *((Uint8*)entry);
    return firstByte == 0x00 || firstByte == 0xE5;
}

void fat32_directoryEntry_parse(FAT32UnknownTypeEntry* entriesBegin, String* nameOut, Flags8* attributeOut, FSnodeAttribute* fsnodeAttributeOut, Index32* firstClusterOut, Size* sizeOut);

Size fat32_directoryEntry_initEntries(FAT32UnknownTypeEntry* entriesBegin, ConstCstring name, fsEntryType type, FSnodeAttribute* attribute, Index32 firstCluster, Size size);

#endif // __FS_FAT32_DIRECTORYENTRY_H
