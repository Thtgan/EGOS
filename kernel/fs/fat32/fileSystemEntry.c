#include<fs/fat32/fileSystemEntry.h>

#include<fs/fat32/fat32.h>
#include<fs/fat32/inode.h>
#include<fs/fileSystem.h>
#include<fs/fileSystemEntry.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/kMalloc.h>

static Result __FAT32resize(FileSystemEntry* entry, Size newSizeInByte);

static Result __FAT32readChildren(FileSystemEntry* directory, FileSystemEntryDescriptor* buffer, Size n);

static FileSystemEntryOperations _FAT32fileSystemEntryOperations = {
    .seek           = genericFileSystemEntrySeek,
    .read           = genericFileSystemEntryRead,
    .write          = genericFileSystemEntryWrite,
    .resize         = __FAT32resize,
    .readChildren   = __FAT32readChildren
};

Result FAT32openFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry, FileSystemEntryDescriptor* entryDescripotor) {
    if (genericOpenFileSystemEntry(superBlock, entry, entryDescripotor) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    entry->operations = &_FAT32fileSystemEntryOperations;

    return RESULT_SUCCESS;
}

#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY FLAG8(0)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN    FLAG8(1)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_SYSTEM    FLAG8(2)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_VOLUME_ID FLAG8(3)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY FLAG8(4)
#define __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_ARCHIVE   FLAG8(5)

typedef struct {
    char    name[11];
    Uint8   attribute;
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

#define __FAT32_LONG_NAME_ENTRY_FLAG                FLAG8(6)
#define __FAT32_LONG_NAME_ENTRY_ATTRIBUTE           (__FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY | __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN | __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_SYSTEM | __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_VOLUME_ID)
#define __FAT32_LONG_NAME_ENTRY_LEN1                5
#define __FAT32_LONG_NAME_ENTRY_LEN2                6
#define __FAT32_LONG_NAME_ENTRY_LEN3                2
#define __FAT32_LONG_NAME_ENTRY_LEN                 (__FAT32_LONG_NAME_ENTRY_LEN1 + __FAT32_LONG_NAME_ENTRY_LEN2 + __FAT32_LONG_NAME_ENTRY_LEN3)
#define __FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH 255

typedef struct {
    Uint8   order;
    Uint16  doubleBytes1[5];
    Uint8   attribute;
    Uint8   type;
    Uint8   checksum;
    Uint16  doubleBytes2[6];
    Uint16  reserved;
    Uint16  doubleBytes3[2];
} __attribute__((packed)) __FAT32LongNameEntry;

static Result __FAT32resize(FileSystemEntry* entry, Size newSizeInByte) {
    iNode* iNode = entry->iNode;
    FAT32info* info = (FAT32info*)iNode->superBlock->specificInfo;
    FAT32BPB* BPB = info->BPB;
    FAT32iNodeInfo* iNodeInfo = (FAT32iNodeInfo*)iNode->specificInfo;
    Size newSizeInBlock = ALIGN_UP(ALIGN_UP_SHIFT(newSizeInByte, iNode->superBlock->device->bytePerBlockShift), BPB->sectorPerCluster);

    if (newSizeInBlock < iNode->sizeInBlock) {
        Index32 tail = FAT32getCluster(info, iNodeInfo->firstCluster, newSizeInBlock / BPB->sectorPerCluster - 1);
        if (FAT32getClusterType(info, tail) != FAT32_CLUSTER_TYPE_ALLOCATERD || PTR_TO_VALUE(32, info->FAT + tail) == FAT32_END_OF_CLUSTER_CHAIN) {
            return RESULT_FAIL;
        }

        Index32 cut = FAT32cutClusterChain(info, tail);
        FAT32releaseClusterChain(info, cut);
    } else if (newSizeInBlock > iNode->sizeInBlock) {
        Index32 freeClusterChain = FAT32allocateClusterChain(info, (newSizeInBlock - iNode->sizeInBlock) / BPB->sectorPerCluster);
        Index32 tail = FAT32getCluster(info, iNodeInfo->firstCluster, iNode->sizeInBlock / BPB->sectorPerCluster - 1);

        if (FAT32getClusterType(info, tail) != FAT32_CLUSTER_TYPE_ALLOCATERD || FAT32getClusterType(info, freeClusterChain) != FAT32_CLUSTER_TYPE_ALLOCATERD || PTR_TO_VALUE(32, info->FAT + tail) != FAT32_END_OF_CLUSTER_CHAIN) {
            return RESULT_FAIL;
        }

        FAT32insertClusterChain(info, tail, freeClusterChain);
    }

    iNode->sizeInBlock = newSizeInBlock;
    entry->descriptor->dataRange.length = newSizeInByte;

    return RESULT_SUCCESS;
}

static Result __doFAT32readChildren(FileSystemEntry* directory, FileSystemEntryDescriptor* descs, char* buffer, Size n);

static Result __FAT32readChildren(FileSystemEntry* directory, FileSystemEntryDescriptor* descs, Size n) {
    void* buffer = allocateBuffer(BUFFER_SIZE_512);
    if (buffer == NULL) {
        return RESULT_FAIL;
    }

    Result res = __doFAT32readChildren(directory, descs, buffer, n);
    releaseBuffer(buffer, BUFFER_SIZE_512);

    return res;
}

static Result __doFAT32readChildren(FileSystemEntry* directory, FileSystemEntryDescriptor* descs, char* buffer, Size n) {
    for (int i = 0; i < n; ++i) {
        FileSystemEntryDescriptor* desc = descs + i;
        if (rawFileSystemEntryRead(directory, buffer, sizeof(__FAT32DirectoryEntry)) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        FileSystemEntryDescriptorInitArgs args;
        if (buffer[0] == 0x00 || buffer[0] == 0xE5) {
            args = (FileSystemEntryDescriptorInitArgs) {
                .name       = NULL,
                .type       = FILE_SYSTEM_ENTRY_TYPE_DUMMY,
                .dataRange  = RANGE(FILE_SYSTEM_ENTRY_INVALID_POSITION, FILE_SYSTEM_ENTRY_INVALID_SIZE),
                .parent     = directory->descriptor,
                .flags      = EMPTY_FLAGS
            };

            initFileSystemEntryDescriptor(desc, &args);

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

        args = (FileSystemEntryDescriptorInitArgs) {
            .name       = nameBuffer,
            .type       = type,
            .dataRange  = RANGE(
                (clusterBegin * BPB->sectorPerCluster + info->dataBlockRange.begin) << superBlock->device->bytePerBlockShift,
                type == FILE_SYSTEM_ENTRY_TYPE_FILE ? directoryEntry->size : ((FAT32getClusterChainLength(info, clusterBegin) * BPB->sectorPerCluster) << superBlock->device->bytePerBlockShift)
            ),
            .parent     = directory->descriptor,
            .flags      = EMPTY_FLAGS
        };

        if (TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY)) {
            SET_FLAG_BACK(args.flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_READ_ONLY);
        }

        if (TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN)) {
            SET_FLAG_BACK(args.flags, FILE_SYSTEM_ENTRY_DESCRIPTOR_FLAGS_HIDDEN);
        }

        initFileSystemEntryDescriptor(desc, &args);

        rawFileSystemEntrySeek(directory, directory->pointer + entrySize);
    }

    return RESULT_CONTINUE;
}