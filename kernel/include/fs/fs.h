#if !defined(__FS_FS_H)
#define __FS_FS_H

typedef enum {
    FS_TYPE_FAT32,
    FS_TYPE_DEVFS,
    FS_TYPE_NUM,
    FS_TYPE_UNKNOWN
} FStype;

typedef struct FS FS;

#include<devices/block/blockDevice.h>
#include<fs/inode.h>
#include<fs/superblock.h>
#include<kit/oop.h>
#include<kit/types.h>

typedef struct FS {
    SuperBlock*     superBlock;
    ConstCstring    name;
    FStype          type;
} FS;

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

//Non-FS codes should use these functions

Result fs_fileRead(File* file, void* buffer, Size n);

Result fs_fileWrite(File* file, const void* buffer, Size n);

#define FS_FILE_SEEK_BEGIN      0
#define FS_FILE_SEEK_CURRENT    1
#define FS_FILE_SEEK_END        2

Index64 fs_fileSeek(File* file, Int64 offset, Uint8 begin);

Result fs_fileOpen(File* file, ConstCstring filename, FCNTLopenFlags flags);

Result fs_fileClose(File* file);

#endif // __FS_FS_H
