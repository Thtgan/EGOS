#include<fs/phospherus/directory.h>

#include<blowup.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/file.h>
#include<fs/phospherus/phospherus.h>
#include<memory/buffer.h>
#include<memory/memory.h>
#include<memory/malloc.h>
#include<stdbool.h>
#include<stdio.h>
#include<string.h>

typedef struct {
    size_t entryNum;
    bool isRoot;
    uint8_t reserved[55];
} __attribute__((packed)) DirectoryHeader;

//Entry may refer to a file (Directory is file, a special file)
typedef struct {
    block_index_t inodeBlockIndex;
    bool isDirectory;
    char name[PHOSPHERUS_MAX_NAME_LENGTH + 1];
} __attribute__((packed)) DirectoryEntry;

block_index_t phospherus_createDirectory(Phospherus_Allocator* allocator) {
    block_index_t ret = phospherus_createFile(allocator);

    File* directoryFile = phospherus_openFile(allocator, ret);
    DirectoryHeader* header = allocateBuffer(BUFFER_SIZE_64);
    header->entryNum = 0;
    header->isRoot = false;

    phospherus_seekFile(directoryFile, 0);
    phospherus_writeFile(directoryFile, header, sizeof(DirectoryHeader));

    phospherus_closeFile(directoryFile);

    releaseBuffer(header, BUFFER_SIZE_64);

    return ret;
}

block_index_t phospherus_createRootDirectory(Phospherus_Allocator* allocator) {
    block_index_t ret = phospherus_createFile(allocator);

    File* directoryFile = phospherus_openFile(allocator, ret);
    DirectoryHeader* header = allocateBuffer(BUFFER_SIZE_64);
    header->entryNum = 0;
    header->isRoot = true;

    phospherus_seekFile(directoryFile, 0);
    phospherus_writeFile(directoryFile, header, sizeof(DirectoryHeader));

    phospherus_closeFile(directoryFile);

    releaseBuffer(header, BUFFER_SIZE_64);

    return ret;
}

bool phospherus_deleteDirectory(Phospherus_Allocator* allocator, block_index_t inodeIndex) {
    phospherus_deleteFile(allocator, inodeIndex);
}

Phospherus_Directory* phospherus_openDirectory(Phospherus_Allocator* allocator, block_index_t inodeIndex) {
    Phospherus_Directory* ret = malloc(sizeof(Phospherus_Directory));

    ret->file = phospherus_openFile(allocator, inodeIndex);
    if (ret->file == NULL) {
        free(ret);
        return NULL;
    }
    ret->allocator = allocator;

    ret->header = malloc(sizeof(DirectoryHeader));
    phospherus_seekFile(ret->file, 0);
    phospherus_readFile(ret->file, ret->header, sizeof(DirectoryHeader));
    size_t entryNum = ((DirectoryHeader*)ret->header)->entryNum;

    ret->entries = NULL;
    if (entryNum > 0) {
        ret->entries = malloc(entryNum * sizeof(DirectoryEntry));
        phospherus_seekFile(ret->file, sizeof(DirectoryHeader));
        phospherus_readFile(ret->file, ret->entries, entryNum * sizeof(DirectoryEntry));
    }
    
    return ret;
}

void phospherus_closeDirectory(Phospherus_Directory* directory) {
    phospherus_closeFile(directory->file);

    free(directory->header);

    if (directory->entries != NULL) {
        free(directory->entries);
    }

    free(directory);
}

size_t phospherus_getDirectoryEntryNum(Phospherus_Directory* directory) {
    DirectoryHeader* header = directory->header;
    return header->entryNum;
}

size_t phospherus_directorySearchEntry(Phospherus_Directory* directory, const char* name, bool isDirectory) {
    size_t entryNum = phospherus_getDirectoryEntryNum(directory);
    DirectoryEntry* entries = directory->entries;
    for (size_t i = 0; i < entryNum; ++i) {
        if (strcmp(name, entries[i].name) == 0 && entries[i].isDirectory == isDirectory) {
            return i;
        }
    }

    return PHOSPHERUS_NULL;
}

static char* _directoryAddEntryFailInfo = "Directory add entry failed\n";

bool phospherus_directoryAddEntry(Phospherus_Directory* directory, block_index_t inodeIndex, const char* name, bool isDirectory) {
    if (phospherus_directorySearchEntry(directory, name, isDirectory) != PHOSPHERUS_NULL) {
        return false;
    }

    DirectoryHeader* header = directory->header;

    if (header->entryNum == 0) {
        directory->entries = malloc(sizeof(DirectoryEntry));
    } else {
        directory->entries = realloc(directory->entries, (header->entryNum + 1) * sizeof(DirectoryEntry));
    }
    ++header->entryNum;
    DirectoryEntry* entryPtr = directory->entries + (header->entryNum - 1) * sizeof(DirectoryEntry);

    entryPtr->inodeBlockIndex = inodeIndex;
    entryPtr->isDirectory = isDirectory;
    strcpy(entryPtr->name, name);

    phospherus_seekFile(directory->file, 0);
    if (!phospherus_writeFile(directory->file, directory->header, sizeof(DirectoryHeader))) {
        blowup(_directoryAddEntryFailInfo);
    }

    phospherus_seekFile(directory->file, sizeof(DirectoryHeader) + (header->entryNum - 1) * sizeof(DirectoryEntry));
    if (!phospherus_writeFile(directory->file, entryPtr, sizeof(DirectoryEntry))) {
        blowup(_directoryAddEntryFailInfo);
    }

    return true;
}

static const char* _deleteDirectoryFailInfo = "Delete directory failed\n";

void phospherus_deleteDirectoryEntry(Phospherus_Directory* directory, size_t entryIndex) {
    DirectoryHeader* header = directory->header;

    if (header->entryNum == 1) {
        free(directory->entries);
        directory->entries = NULL;
    } else {
        memmove(directory->entries + entryIndex * sizeof(DirectoryEntry), directory->entries + (entryIndex + 1) * sizeof(DirectoryEntry), (header->entryNum - entryIndex - 1) * sizeof(DirectoryEntry));
        directory->entries = realloc(directory->entries, (header->entryNum - 1) * sizeof(DirectoryEntry));
    }
    --header->entryNum;

    phospherus_seekFile(directory->file, 0);
    if (!phospherus_writeFile(directory->file, directory->header, sizeof(DirectoryHeader))) {
        blowup(_deleteDirectoryFailInfo);
    }

    if (header->entryNum != 0) {
        phospherus_seekFile(directory->file, sizeof(DirectoryHeader));
        phospherus_writeFile(directory->file, directory->entries + entryIndex * sizeof(DirectoryEntry), (header->entryNum - entryIndex) * sizeof(DirectoryEntry));
    }

    if (!phospherus_truncateFile(directory->file, sizeof(DirectoryHeader) + header->entryNum * sizeof(DirectoryEntry))) {
        blowup(_deleteDirectoryFailInfo);
    }
}

const char* phospherus_getDirectoryEntryName(Phospherus_Directory* directory, size_t entryIndex) {
    DirectoryEntry* entries = directory->entries;
    return entries[entryIndex].name;
}

bool phospherus_setDirectoryEntryName(Phospherus_Directory* directory, size_t entryIndex, const char* name) {
    DirectoryEntry* entries = directory->entries;
    if (strlen(name) > PHOSPHERUS_MAX_NAME_LENGTH) {
        return false;
    }
    strcpy(entries[entryIndex].name, name);

    return true;
}

block_index_t phospherus_getDirectoryEntryInodeIndex(Phospherus_Directory* directory, size_t entryIndex) {
    DirectoryEntry* entries = directory->entries;
    return entries[entryIndex].inodeBlockIndex;
}

bool phospherus_checkDirectoryEntryIsDirectory(Phospherus_Directory* directory, size_t entryIndex) {
    DirectoryEntry* entries = directory->entries;
    return entries[entryIndex].isDirectory;
}

bool phospherus_checkIsRootDirectory(Phospherus_Directory* directory) {
    DirectoryHeader* header = directory->header;
    return header->isRoot;
}