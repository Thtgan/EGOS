#if !defined(__PHOSPHERUS_H)
#define __PHOSPHERUS_H

#include<devices/block/blockDevice.h>
#include<fs/fileSystem.h>
#include<fs/phospherus/allocator.h>
#include<stdbool.h>

//TODO: Move this to independent header as a null index pointer
#define PHOSPHERUS_NULL             -1
#define PHOSPHERUS_MAX_NAME_LENGTH  54

/**
 * @brief Initialize the phospherus file system
 */
void phospherus_initFileSystem();

/**
 * @brief Deploy phospherus file system on device
 * 
 * @param device Device to mount the phospherus file system
 * @return Is deployment succeeded
 */
bool phospherus_deployFileSystem(BlockDevice* device);

/**
 * @brief Check if the device has phospherus deployed
 * 
 * @param device Device to check
 * @return Is the device has phospherus deployed
 */
bool phospherus_checkFileSystem(BlockDevice* device);

/**
 * @brief Open phospherus file system on device
 * 
 * @param device Device has phospherus deployed
 * @return FileSystem* OPened file system
 */
FileSystem* phospherus_openFileSystem(BlockDevice* device);

/**
 * @brief Close a opened phospherus file system, cannot close a closed file system
 * 
 * @param system Fiel system to close
 */
void phospherus_closeFileSystem(FileSystem* system);

#endif // __PHOSPHERUS_H
