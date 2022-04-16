#if !defined(__PHOSPHERUS_DIRECTORY_H)
#define __PHOSPHERUS_DIRECTORY_H

#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/allocator.h>
#include<fs/phospherus/file.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>

//Directory == File

typedef struct {
    void* header;
    void* entries;
    File* file;
    Phospherus_Allocator* allocator;
} Phospherus_Directory;

block_index_t phospherus_createDirectory(Phospherus_Allocator* allocator);

block_index_t phospherus_createRootDirectory(Phospherus_Allocator* allocator);

bool phospherus_deleteDirectory(Phospherus_Allocator* allocator, block_index_t inodeIndex);

Phospherus_Directory* phospherus_openDirectory(Phospherus_Allocator* allocator, block_index_t inodeIndex);

void phospherus_closeDirectory(Phospherus_Directory* directory);



size_t phospherus_getDirectoryEntryNum(Phospherus_Directory* directory);

size_t phospherus_directorySearchEntry(Phospherus_Directory* directory, const char* name, bool isDirectory);

bool phospherus_directoryAddEntry(Phospherus_Directory* directory, block_index_t inodeIndex, const char* name, bool isDirectory);

void phospherus_deleteDirectoryEntry(Phospherus_Directory* directory, size_t entryIndex);



const char* phospherus_getDirectoryEntryName(Phospherus_Directory* directory, size_t entryIndex);

bool phospherus_setDirectoryEntryName(Phospherus_Directory* directory, size_t entryIndex, const char* name);

block_index_t phospherus_getDirectoryEntryInodeIndex(Phospherus_Directory* directory, size_t entryIndex);

bool phospherus_checkDirectoryEntryIsDirectory(Phospherus_Directory* directory, size_t entryIndex);

bool phospherus_checkIsRootDirectory(Phospherus_Directory* directory);

#endif // __PHOSPHERUS_DIRECTORY_H
