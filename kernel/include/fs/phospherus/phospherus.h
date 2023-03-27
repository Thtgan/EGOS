#if !defined(__PHOSPHERUS_H)
#define __PHOSPHERUS_H

#include<devices/block/blockDevice.h>
#include<fs/fileSystem.h>
#include<fs/phospherus/allocator.h>
#include<kit/types.h>
#include<returnValue.h>

/**
 * @brief Initialize the phospherus file system
 */
void phospherusInitFileSystem();

/**
 * @brief Deploy phospherus file system on device
 * 
 * @param device Device to mount the phospherus file system
 * @return Is deployment succeeded
 */
ReturnValue phospherusDeployFileSystem(BlockDevice* device);

/**
 * @brief Check if the device has phospherus deployed
 * 
 * @param device Device to check
 * @return Is the device has phospherus deployed
 */
ReturnValue phospherusCheckFileSystem(BlockDevice* device);

/**
 * @brief Open phospherus file system on device
 * 
 * @param device Device has phospherus deployed
 * @return FileSystem* OPened file system
 */
FileSystem* phospherusOpenFileSystem(BlockDevice* device);

/**
 * @brief Close a opened phospherus file system, cannot close a closed file system
 * 
 * @param fs Fiel system to close
 */
void phospherusCloseFileSystem(FileSystem* fs);

#endif // __PHOSPHERUS_H
