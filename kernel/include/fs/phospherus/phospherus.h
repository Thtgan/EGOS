#if !defined(__PHOSPHERUS_H)
#define __PHOSPHERUS_H

#include<fs/blockDevice/blockDevice.h>
#include<fs/fileSystem.h>
#include<fs/phospherus/allocator.h>
#include<stdbool.h>

//TODO: Move this to independent header as a null index pointer
#define PHOSPHERUS_NULL             -1
#define PHOSPHERUS_MAX_NAME_LENGTH  54

void phospherus_initFileSystem();

bool phospherus_deployFileSystem(BlockDevice* device);

bool phospherus_checkFileSystem(BlockDevice* device);

FileSystem* phospherus_openFileSystem(BlockDevice* device);

void phospherus_closeFileSystem(FileSystem* system);

#endif // __PHOSPHERUS_H
