#if !defined(__FS_H)
#define __FS_H

#include<devices/block/blockDevice.h>
#include<fs/fsStructs.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/oop.h>
#include<kit/types.h>

#define FS_PATH_SEPERATOR '/'

/**
 * @brief Initialize the file system
 * 
 * @return Result Result of the operation
 */
Result fs_init();

/**
 * @brief Check type of file system on device
 * 
 * @param device Device to check
 * @return FileSystemTypes Type of file system
 */
FStype fs_checkType(BlockDevice* device);

/**
 * @brief Open file system on device, return from buffer if file system is open, sets errorcode to indicate error
 * 
 * @param device Device to open
 * @return FS* File system, NULL if error happens
 */
Result fs_open(FS* fs, BlockDevice* device);

/**
 * @brief Close a opened file system, cannot close a closed file system, sets errorcode to indicate error
 * 
 * @param system File system
 * @return Result Result of the operation, NULL if error happens
 */
Result fs_close(FS* fs);

#endif // __FS_H
