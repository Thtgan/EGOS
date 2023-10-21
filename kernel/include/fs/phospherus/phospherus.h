#if !defined(__PHOSPHERUS_H)
#define __PHOSPHERUS_H

#include<devices/block/blockDevice.h>
#include<fs/fileSystem.h>
#include<fs/phospherus/allocator.h>
#include<kit/types.h>

/**
 * @brief Initialize the phospherus file system, sets errorcode to indicate error
 * @return Result Result of the operation
 */
Result phospherusInitFileSystem();

/**
 * @brief Deploy phospherus file system on device, sets errorcode to indicate error
 * 
 * @param device Device to mount the phospherus file system
 * @return Result Result of the operation
 */
Result phospherusDeployFileSystem(BlockDevice* device);

/**
 * @brief Check if the device has phospherus deployed, sets errorcode to indicate error
 * 
 * @param device Device to check
 * @return bool Is file system on this device
 */
bool phospherusCheckFileSystem(BlockDevice* device);

/**
 * @brief Open phospherus file system on device, sets errorcode to indicate error
 * 
 * @param device Device has phospherus deployed
 * @return FileSystem* Opened file system, NULL if failed
 */
FileSystem* phospherusOpenFileSystem(BlockDevice* device);

/**
 * @brief Close a opened phospherus file system, cannot close a closed file system, sets errorcode to indicate error
 * 
 * @param fs File system to close
 * @return Result Result of the operation
 */
Result phospherusCloseFileSystem(FileSystem* fs);

#endif // __PHOSPHERUS_H
