#include<fs/fat32/fat32.h>

#include<fs/volume.h>
#include<fs/fileSystem.h>
#include<kit/types.h>
#include<kit/util.h>
#include<lib/memory.h>
#include<lib/string.h>
#include<mm/mm.h>

typedef struct {
    Uint8   jump[3];
    char    oem[8];
    Uint16  bytePerSector;
    Uint8   sectorPerCluster;
    Uint16  reservedSectorNum;
    Uint8   FATnum;
    Uint16  rootDirectoryEntryNum;
    Uint16  sectorNum;
    Uint8   media;
    Uint16  reserved1;
    Uint16  sectorPerTrack;
    Uint16  headNum;
    Uint32  hiddenSectorNum;
    Uint32  sectorNumLarge;
    Uint32  sectorPerFAT;
    Uint16  flags;
    Uint16  version;
    Uint32  rootDirectoryClusterNum;
    Uint16  FSinfoSectorNum;
    Uint16  backupBootSectorNum;
    Uint8   reserved2[12];
    Uint8   drive;
    Uint8   winNTflags;
    Uint8   signature;
    Uint32  volumeSerialNumber;
    char    label[11];
    char    systemIdentifier[8];
} __attribute__((packed)) __FAT32BPB;

typedef struct {
    Uint8   sectorPerCluster;
    Uint16  reservedSectorNum;
    Uint32  hiddenSectorNum;

    RangeN  FATrange;
    Range   rootDirectoryRange;
    Range   dataRange;

    Uint32  clusterNum;
} __FAT32Context;

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

#define __FAT32_LONG_NAME_ENTRY_FLAG        FLAG8(6)
#define __FAT32_LONG_NAME_ENTRY_ATTRIBUTE   (__FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY | __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN | __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_SYSTEM | __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_VOLUME_ID)
#define __FAT32_LONG_NAME_ENTRY_LEN1        5
#define __FAT32_LONG_NAME_ENTRY_LEN2        6
#define __FAT32_LONG_NAME_ENTRY_LEN3        2
#define __FAT32_LONG_NAME_ENTRY_LEN         (__FAT32_LONG_NAME_ENTRY_LEN1 + __FAT32_LONG_NAME_ENTRY_LEN2 + __FAT32_LONG_NAME_ENTRY_LEN3)

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

static Result __doFAT32checkFileSystem(Volume* v, void* BPBbuffer);

static Result __doFAT32openFileSystem(Volume* v, FileSystem* fileSystem, void* BPBbuffer, __FAT32Context* context);

Result FAT32checkFileSystem(Volume* v) {
    void* BPBbuffer = bMalloc(v->bytePerSector);
    if (BPBbuffer == NULL) {
        return RESULT_FAIL;
    }
    
    Result ret = __doFAT32checkFileSystem(v, BPBbuffer);

    bFree(BPBbuffer, v->bytePerSector);
    return ret;
}

static ConstCstring _name = "FAT32";

Result FAT32openFileSystem(Volume* v, FileSystem* fileSystem) {
    void* BPBbuffer = bMalloc(v->bytePerSector);
    if (BPBbuffer == NULL) {
        return RESULT_FAIL;
    }

    void* context = bMalloc(sizeof(__FAT32Context));
    Result ret = RESULT_FAIL;
    if (context == NULL || (ret = __doFAT32openFileSystem(v, fileSystem, BPBbuffer, context)) == RESULT_FAIL) {
        if (context != NULL) {
            bFree(context, sizeof(__FAT32Context));
        }
    }

    bFree(BPBbuffer, v->bytePerSector);
    return ret;
}

#define __FAT32_BPB_SIGNATURE       0x29
#define __FAT32_MINIMUM_CLUSTER_NUM 65525

static Result __doFAT32checkFileSystem(Volume* v, void* BPBbuffer) {
    if (rawVolumeReadSectors(v, BPBbuffer, 0, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    __FAT32BPB* BPB = (__FAT32BPB*)BPBbuffer;

    Uint32 rootDirectorySectorNum = DIVIDE_ROUND_UP(32 * BPB->rootDirectoryEntryNum, BPB->bytePerSector);
    Uint32 clusterNum = (v->sectorNum - BPB->reservedSectorNum - BPB->FATnum * BPB->sectorPerFAT - rootDirectorySectorNum) / BPB->sectorPerCluster;
    if (BPB->bytePerSector == v->bytePerSector && BPB->signature == __FAT32_BPB_SIGNATURE && memcmp(BPB->systemIdentifier, "FAT32   ", 8) == 0 && clusterNum > __FAT32_MINIMUM_CLUSTER_NUM) {
        return RESULT_SUCCESS;
    }

    return RESULT_FAIL;
}

//================================================================================================================================

typedef enum {
    FAT32_CLUSTER_TYPE_FREE,
    FAT32_CLUSTER_TYPE_ALLOCATERD,
    FAT32_CLUSTER_TYPE_RESERVED,
    FAT32_CLUSTER_TYPE_BAD,
    FAT32_CLUSTER_TYPE_EOF,
    FAT32_CLUSTER_TYPE_NOT_CLUSTER
} __FAT32ClusterType;

static __FAT32ClusterType __FAT32GetClusterType(__FAT32Context* context, Index32 cluster);

static Index32 __FAT32GetNextCluster(Volume* v, Index32 cluster);

static Result __FAT32ReadThroughClusters(Volume* v, void* buffer, Index32 cluster, Index32 offsetInCluster, Size n);

static Result __doFAT32ReadThroughClusters(Volume* v, void* buffer, Index32 cluster, Index32 offsetInCluster, Size n, void* clusterBuffer);

//================================================================================================================================

static Result __FAT32FileSystemEntryOpen(Volume* v, ConstCstring path, FileSystemEntry* entry, FileSystemEntryType type);

static void __FAT32FileSystemEntryClose(FileSystemEntry* entry);

static FileSystemOperations _FAT32FileSystemOperations = {
    .fileSystemEntryOpen    = __FAT32FileSystemEntryOpen,
    .fileSystemEntryClose   = __FAT32FileSystemEntryClose
};

//================================================================================================================================

static Index32 __FAT32FileSeek(FileSystemEntry* file, Index32 seekTo);

static Result __FAT32FileRead(FileSystemEntry* file, void* buffer, Size n);

static FileOperations _FAT32FileOperations = {
    .seek   = __FAT32FileSeek,
    .read   = __FAT32FileRead
};

//================================================================================================================================

static Result __FAT32DirectoryLookupEntry(FileSystemEntry* directory, ConstCstring name, FileSystemEntryType type, FileSystemEntry* entry, Index32* entryIndex);

static DirectoryOperations _FAT32DirectoryOperations = {
    .lookupEntry    = __FAT32DirectoryLookupEntry
};

//================================================================================================================================

typedef struct {
    Index32 dataSectorBegin;
    Size    size;
    Index32 pointer;
} __FAT32FileHandle;

typedef struct {
    Index32 dataSectorBegin;
} __FAT32DirectoryHandle;

//================================================================================================================================

static Result __FAT32ReadDirectoryEntry(Volume* v, Index32 cluster, Index32 offsetInCluster, FileSystemEntry* entry, Size* entrySizeInDevice);

static Result __FAT32ConvertDirectoryEntry(Volume* v, FileSystemEntry* entry, void* entryBuffer, Uint8 longNameEntryNum);

//================================================================================================================================

static __FAT32ClusterType __FAT32GetClusterType(__FAT32Context* context, Index32 cluster) {
    if (cluster == 0x00000000) {
        return FAT32_CLUSTER_TYPE_FREE;
    }

    if (0x00000002 <= cluster && cluster < context->clusterNum) {
        return FAT32_CLUSTER_TYPE_ALLOCATERD;
    }

    if (context->clusterNum <= cluster && cluster < 0x0FFFFFF7) {
        return FAT32_CLUSTER_TYPE_RESERVED;
    }

    if (cluster == 0x0FFFFFF7) {
        return FAT32_CLUSTER_TYPE_BAD;
    }

    if (0x0FFFFFF8 <= cluster && cluster < 0x0FFFFFFE) {
        return FAT32_CLUSTER_TYPE_RESERVED;
    }

    if (cluster == 0x0FFFFFFF) {
        return FAT32_CLUSTER_TYPE_EOF;
    }

    return FAT32_CLUSTER_TYPE_NOT_CLUSTER;
}

static Index32 __FAT32GetNextCluster(Volume* v, Index32 cluster) {
    __FAT32Context* context = ((FileSystem*)v->fileSystem)->context;
    if (cluster >= context->clusterNum) {
        return INVALID_INDEX;
    }

    Index32 sector = context->FATrange.begin + cluster * 4 / v->bytePerSector;
    Index32 offset = cluster * 4 % v->bytePerSector;

    void* FATbuffer = bMalloc(v->bytePerSector);
    if (FATbuffer == NULL) {
        return INVALID_INDEX;
    }

    Index32 ret = INVALID_INDEX;
    if (rawVolumeReadSectors(v, FATbuffer, sector, 1) == RESULT_SUCCESS) {
        ret = PTR_TO_VALUE(32, FATbuffer + offset) & 0x0FFFFFFF;
    }
    bFree(FATbuffer, v->bytePerSector);

    return ret;
}

static Result __FAT32ReadThroughClusters(Volume* v, void* buffer, Index32 cluster, Index32 offsetInCluster, Size n) {
    __FAT32Context* context = ((FileSystem*)v->fileSystem)->context;
    Size clusterSize = context->sectorPerCluster * v->bytePerSector;

    void* clusterBuffer = bMalloc(clusterSize);
    if (clusterBuffer == NULL) {
        return RESULT_FAIL;
    }

    Result ret = __doFAT32ReadThroughClusters(v, buffer, cluster, offsetInCluster, n, clusterBuffer);
    bFree(clusterBuffer, clusterSize);

    return ret;
}

static Result __doFAT32ReadThroughClusters(Volume* v, void* buffer, Index32 cluster, Index32 offsetInCluster, Size n, void* clusterBuffer) {
    __FAT32Context* context = ((FileSystem*)v->fileSystem)->context;

    Size remain = n, clusterSize = context->sectorPerCluster * v->bytePerSector;
    void* currentBuffer = buffer;
    Index32 currentCluster = cluster, currentOffset = offsetInCluster;

    while (remain > 0) {
        if (rawVolumeReadSectors(v, clusterBuffer, context->dataRange.begin + currentCluster * context->sectorPerCluster, context->sectorPerCluster) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        Size bytesToRead = clusterSize - currentOffset;
        if (bytesToRead > remain) {
            bytesToRead = remain;
        }

        memcpy(currentBuffer, clusterBuffer + currentOffset, bytesToRead);
        
        currentBuffer += bytesToRead;
        remain -= bytesToRead;

        currentOffset = 0;
        Index32 nextCluster = __FAT32GetNextCluster(v, currentCluster);
        if (remain > 0 && __FAT32GetClusterType(context, nextCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            return RESULT_FAIL;
        }
        currentCluster = nextCluster;
    }

    return RESULT_SUCCESS;
}

//================================================================================================================================

#define __FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH 255

static Result __FAT32FileSystemEntryOpen(Volume* v, ConstCstring path, FileSystemEntry* entry, FileSystemEntryType type) {
    if (((FileSystem*)v->fileSystem)->type != FILE_SYSTEM_TYPE_FAT32) {
        return RESULT_FAIL;
    }

    while (*path == '/') {
        ++path;
    }

    FileSystemEntry currentDirectory, nextEntry;

    memset(&currentDirectory, 0, sizeof(FileSystemEntry));
    currentDirectory.volume                 = v;
    currentDirectory.flags                  = EMPTY_FLAGS;
    currentDirectory.type                   = FILE_SYSTEM_ENTRY_TYPE_DIRECTOY;
    currentDirectory.directoryOperations    = &_FAT32DirectoryOperations;

    __FAT32Context* context = ((FileSystem*)v->fileSystem)->context;
    currentDirectory.context                = context;

    __FAT32DirectoryHandle* handle = bMalloc(sizeof(__FAT32DirectoryHandle));
    if (handle == NULL) {
        return RESULT_FAIL;
    }
    handle->dataSectorBegin = context->rootDirectoryRange.begin;
    currentDirectory.handle                 = handle;

    char buffer[__FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH + 1];
    while (*path != '\0') {
        int componentLen = 0;
        for (; *path != '/' && *path != '\0'; ++componentLen, ++path) {
            buffer[componentLen] = *path;
        }
        buffer[componentLen] = '\0';

        bool isTarget = (*path++ == '\0');

        if (__FAT32DirectoryLookupEntry(&currentDirectory, buffer, isTarget ? type : FILE_SYSTEM_ENTRY_TYPE_DIRECTOY, &nextEntry, NULL) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (isTarget) {
            *entry = nextEntry;
            break;
        } else {
            __FAT32FileSystemEntryClose(&currentDirectory);
            currentDirectory = nextEntry;
        }
    }

    return RESULT_SUCCESS;
}

static void __FAT32FileSystemEntryClose(FileSystemEntry* entry) {
    if (entry->name != NULL) {
        bFree((void*)entry->name, entry->nameBufferSize);
    }

    if (entry->handle != NULL) {
        bFree(entry->handle, entry->type == FILE_SYSTEM_ENTRY_TYPE_FILE ? sizeof(__FAT32FileHandle) : sizeof(__FAT32DirectoryHandle));
    }
}

static Index32 __FAT32FileSeek(FileSystemEntry* file, Index32 seekTo) {
    __FAT32FileHandle* handle = file->handle;
    return handle->pointer = seekTo < handle->size ? seekTo : INVALID_INDEX;
}

static Result __FAT32FileRead(FileSystemEntry* file, void* buffer, Size n) {
    __FAT32Context* context = file->context;
    Size clusterSize = context->sectorPerCluster * file->volume->bytePerSector;

    __FAT32FileHandle* handle = file->handle;
    if (handle->pointer >= handle->size) {
        return RESULT_SUCCESS;
    }

    Size remainByteNum = handle->size - handle->pointer;
    n = n < remainByteNum ? n : remainByteNum;

    Index32 currentCluster = (handle->dataSectorBegin - context->dataRange.begin) / context->sectorPerCluster, currentOffset = ((handle->dataSectorBegin - context->dataRange.begin) % context->sectorPerCluster) * file->volume->bytePerSector + handle->pointer;

    while (currentOffset >= clusterSize) {
        currentCluster = __FAT32GetNextCluster(file->volume, currentCluster);
        if (__FAT32GetClusterType(context, currentCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
            return RESULT_FAIL;
        }
        currentOffset -= clusterSize;
    }

    Result ret = __FAT32ReadThroughClusters(file->volume, buffer, currentCluster, currentOffset, n);

    if (ret == RESULT_SUCCESS) {
        handle->pointer += n;
    }

    return ret;
}

//================================================================================================================================

Result __FAT32DirectoryLookupEntry(FileSystemEntry* directory, ConstCstring name, FileSystemEntryType type, FileSystemEntry* entry, Index32* entryIndex) {
    __FAT32Context* context = directory->context;
    Size clusterSize = context->sectorPerCluster * directory->volume->bytePerSector;

    __FAT32DirectoryHandle* handle = directory->handle;
    Index32 currentCluster = (handle->dataSectorBegin - context->dataRange.begin) / context->sectorPerCluster, currentOffset = ((handle->dataSectorBegin - context->dataRange.begin) % context->sectorPerCluster) * directory->volume->bytePerSector;

    FileSystemEntry currentEntry;
    Index32 index = INVALID_INDEX;
    Size entrySizeInDevice;

    for (int i = 0;; ++i) {
        if (__FAT32ReadDirectoryEntry(directory->volume, currentCluster, currentOffset, &currentEntry, &entrySizeInDevice) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (strcmp(currentEntry.name, name) == 0 && currentEntry.type == type) {
            index = i;
            break;
        }

        if (currentEntry.type == FILE_SYSTEM_ENTRY_TYPE_DUMMY && entrySizeInDevice == -1) {
            break;
        }

        currentOffset += entrySizeInDevice;
        __FAT32FileSystemEntryClose(&currentEntry);
        while (currentOffset >= clusterSize) {
            currentCluster = __FAT32GetNextCluster(directory->volume, currentCluster);
            if (__FAT32GetClusterType(context, currentCluster) != FAT32_CLUSTER_TYPE_ALLOCATERD) {
                return RESULT_FAIL;
            }
            currentOffset -= clusterSize;
        }
    }

    if (entry != NULL) {
        memcpy(entry, &currentEntry, sizeof(FileSystemEntry));
    }

    if (entryIndex != NULL) {
        *entryIndex = index;
    }

    return RESULT_SUCCESS;
}

//================================================================================================================================

static Result __FAT32ReadDirectoryEntry(Volume* v, Index32 cluster, Index32 offsetInCluster, FileSystemEntry* entry, Size* entrySizeInDevice) {
    Uint8 dummyBuffer[32];
    if (__FAT32ReadThroughClusters(v, dummyBuffer, cluster, offsetInCluster, 32) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    __FAT32Context* context = ((FileSystem*)v->fileSystem)->context;

    if (dummyBuffer[0] == 0x00 || dummyBuffer[0] == 0xE5) {
        *entrySizeInDevice = dummyBuffer[0] == 0x00 ? -1 : sizeof(__FAT32DirectoryEntry);
        entry->type = FILE_SYSTEM_ENTRY_TYPE_DUMMY;
        return RESULT_SUCCESS;
    }

    *entrySizeInDevice = 0;

    Uint8 longNameOrder = 0;
    if (((__FAT32LongNameEntry*)dummyBuffer)->attribute == __FAT32_LONG_NAME_ENTRY_ATTRIBUTE) {
        __FAT32LongNameEntry* longNameEntry = (__FAT32LongNameEntry*)(dummyBuffer);

        if (TEST_FLAGS_FAIL(longNameEntry->order, __FAT32_LONG_NAME_ENTRY_FLAG)) {
            return RESULT_FAIL;
        }

        longNameOrder = (Uint8)VAL_XOR(longNameEntry->order, __FAT32_LONG_NAME_ENTRY_FLAG);
    }

    Size entrySize = longNameOrder * sizeof(__FAT32LongNameEntry) + sizeof(__FAT32DirectoryEntry);
    void* entryBuffer = bMalloc(entrySize);
    if (entryBuffer == NULL || __FAT32ReadThroughClusters(v, entryBuffer, cluster, offsetInCluster, entrySize) == RESULT_FAIL) {
        if (entryBuffer) {
            bFree(entryBuffer, entrySize);
        }

        return RESULT_FAIL;
    }

    Result ret = __FAT32ConvertDirectoryEntry(v, entry, entryBuffer, longNameOrder);

    bFree(entryBuffer, entrySize);

    *entrySizeInDevice = entrySize;

    return ret;
}

static Result __FAT32ConvertDirectoryEntry(Volume* v, FileSystemEntry* entry, void* entryBuffer, Uint8 longNameEntryNum) {
    char nameBuffer[__FAT32_DIRECTORY_ENTRY_NAME_MAXIMUM_LENGTH + 1];
    Uint8 nameLength = 0;
    __FAT32DirectoryEntry* directoryEntry = (__FAT32DirectoryEntry*)(entryBuffer + longNameEntryNum * sizeof(__FAT32LongNameEntry));

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
            __FAT32LongNameEntry* longNameEntry = (__FAT32LongNameEntry*)(entryBuffer + i * sizeof(__FAT32LongNameEntry));

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

    Cstring name = bMalloc(nameLength + 1);
    if (name == NULL) {
        return RESULT_FAIL;
    }

    __FAT32Context* context = ((FileSystem*)v->fileSystem)->context;

    memcpy(name, nameBuffer, nameLength);
    name[nameLength] = '\0';

    entry->name                             = name;
    entry->nameBufferSize                   = nameLength + 1;

    entry->volume                           = v;

    entry->flags                            = EMPTY_FLAGS;
    if (TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_READ_ONLY)) {
        SET_FLAG_BACK(entry->flags, FILE_SYSTEM_ENTRY_FLAGS_READ_ONLY);
    }

    if (TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_HIDDEN)) {
        SET_FLAG_BACK(entry->flags, FILE_SYSTEM_ENTRY_FLAGS_HIDDEN);
    }

    entry->type                             = TEST_FLAGS(directoryEntry->attribute, __FAT32_DIRECTORY_ENTRY_ATTRIBUTE_DIRECTORY) ? FILE_SYSTEM_ENTRY_TYPE_DIRECTOY : FILE_SYSTEM_ENTRY_TYPE_FILE;

    entry->context                          = context;
    switch (entry->type) {
        case FILE_SYSTEM_ENTRY_TYPE_FILE: {
            entry->fileOperations           = &_FAT32FileOperations;
            __FAT32FileHandle* handle = bMalloc(sizeof(__FAT32FileHandle));
            if (handle == NULL) {
                bFree(name, nameLength + 1);
                return RESULT_FAIL;
            }
            handle->dataSectorBegin     = (((Uint32)directoryEntry->clusterBeginHigh << 16) | directoryEntry->clusterBeginLow) * context->sectorPerCluster + context->dataRange.begin;
            handle->size                = directoryEntry->size;
            handle->pointer             = 0;
            entry->handle                   = handle;
            break;
        }
        case FILE_SYSTEM_ENTRY_TYPE_DIRECTOY: {
            entry->directoryOperations      = &_FAT32DirectoryOperations;
            __FAT32DirectoryHandle* handle = bMalloc(sizeof(__FAT32DirectoryHandle));
            if (handle == NULL) {
                bFree(name, nameLength + 1);
                return RESULT_FAIL;
            }
            handle->dataSectorBegin     = (((Uint32)directoryEntry->clusterBeginHigh << 16) | directoryEntry->clusterBeginLow) * context->sectorPerCluster + context->dataRange.begin;
            entry->handle                   = handle;
            break;
        }
        default:
            break;
    }

    return RESULT_SUCCESS;
}

//================================================================================================================================

static Result __doFAT32openFileSystem(Volume* v, FileSystem* fileSystem, void* BPBbuffer, __FAT32Context* context) {
    if (rawVolumeReadSectors(v, BPBbuffer, 0, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    __FAT32BPB* BPB = (__FAT32BPB*)BPBbuffer;

    fileSystem->name                    = _name;
    fileSystem->type                    = FILE_SYSTEM_TYPE_FAT32;
    fileSystem->fileSystemOperations    = &_FAT32FileSystemOperations;
    fileSystem->context                 = context;

    context->sectorPerCluster   = BPB->sectorPerCluster;
    context->reservedSectorNum  = BPB->reservedSectorNum;
    context->hiddenSectorNum    = BPB->hiddenSectorNum;

    context->FATrange           = RANGE_N(BPB->FATnum, context->reservedSectorNum, BPB->sectorPerFAT);

    Uint32 rootDirectorryBegin  = context->reservedSectorNum + BPB->FATnum * BPB->sectorPerFAT, rootDirectorryLength = BPB->rootDirectoryClusterNum * BPB->sectorPerCluster;
    context->rootDirectoryRange = RANGE(rootDirectorryBegin, rootDirectorryLength);

    Uint32 dataBegin            = rootDirectorryBegin - BPB->rootDirectoryClusterNum * BPB->sectorPerCluster, dataLength = v->sectorNum - dataBegin;
    context->dataRange          = RANGE(dataBegin, dataLength);

    context->clusterNum         = DIVIDE_ROUND_UP(dataLength, BPB->sectorPerCluster);

    return RESULT_SUCCESS;
}