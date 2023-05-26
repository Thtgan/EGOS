#if !defined(__FS_UTIL_H)
#define __FS_UTIL_H

#include<fs/directory.h>
#include<fs/file.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<kernel.h>
#include<kit/types.h>

/**
 * @brief Trace a path and find directory entry corresponded to the path, starts from root directory, sets errorcode to indicate error
 * 
 * @param entry Direct entry struct
 * @param path Path to trace
 * @param type Type of entry
 * @return Result Result of the operation
 */
Result tracePath(DirectoryEntry* entry, ConstCstring path, iNodeType type);

/**
 * @brief Open file from path, starts from root directory, sets errorcode to indicate error
 * 
 * @param path Path to file
 * @return File opened, NULL if error happens
 */
File* fileOpen(ConstCstring path, Flags8 flags);

/**
 * @brief Close file opened, sets errorcode to indicate error
 * 
 * @param file File to close
 * @return Result Result of the operation
 */
Result fileClose(File* file);

#define FSUTIL_SEEK_BEGIN   0
#define FSUTIL_SEEK_CURRENT 1
#define FSUTIL_SEEK_END     2

/**
 * @brief Seek file pointer to position, sets errorcode to indicate error
 * 
 * @param file File
 * @param offset Offset to the vegin
 * @param begin Begin of the seek
 * @return Result Result of the operation
 */
Result fileSeek(File* file, Int64 offset, Uint8 begin);

/**
 * @brief Get current pointer of file
 * 
 * @param file File
 * @return Index64 Pointer position of file
 */
Index64 fileGetPointer(File* file);

/**
 * @brief Read from file, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to write to
 * @param n Num of byte(s) to read
 * @return Result Result of the operation
 */
Result fileRead(File* file, void* buffer, Size n);

/**
 * @brief Write to file, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to read from
 * @param n Num of byte(s) to write
 * @return Result Result of the operation
 */
Result fileWrite(File* file, const void* buffer, Size n);

/**
 * @brief Create an entry and insert it into directory, be aware its parent directory must be existed, this function cannot be used to create entry with type INODE_TYPE_DEVICE,
 * try registerVirtualDevice from virtualDevice.h, sets errorcode to indicate error
 * 
 * @param path Path to new entry's parent directory
 * @param name Name of new entry
 * @param iNodeID iNode ID of new entry
 * @param type Type of new entry
 * @return Result Result of the operation
 */
Result createEntry(ConstCstring path, ConstCstring name, ID iNodeID, iNodeType type);

/**
 * @brief Delete an entry, sets errorcode to indicate error
 * 
 * @param path Path to entry to delete
 * @param type Type of entry to delete
 * @return Result Result of the operation
 */
Result deleteEntry(ConstCstring path, iNodeType type);

/**
 * @brief Open a file from iNode, sets errorcode to indicate error
 * 
 * @param iNode iNode contains file
 * @return File* File opened, NULL if error happens
 */
File* rawFileOpen(iNode* iNode, Flags8 flags);

/**
 * @brief Close a file opened, sets errorcode to indicate error
 * 
 * @param file File to close
 * @return Result Result of the operation
 */
Result rawFileClose(File* file);

/**
 * @brief Open a directory from iNode, sets errorcode to indicate error
 * 
 * @param iNode iNode contains directory
 * @return Directory* Directory opened, NULL if error happens
 */
Directory* rawDirectoryOpen(iNode* iNode);

/**
 * @brief Close a directory opened, sets errorcode to indicate error
 * 
 * @param directory Directory to close
 * @return Result Result of the operation
 */
Result rawDirectoryClose(Directory* directory);

/**
 * @brief Create an iNode on device, this function cannot be used to create iNode with type INODE_TYPE_DEVICE, sets errorcode to indicate error
 * 
 * @param deviceID Device to create iNode
 * @param type Type of iNode to create
 * @return Index64 Index of block contiains iNode, INVALID_INDEX if error happens
 */
Index64 rawInodeCreate(ID deviceID, iNodeType type);

/**
 * @brief Delete an iNode on device, sets errorcode to indicate error
 * 
 * @param iNodeID Device where iNode is
 * @return Result Result of the operation
 */
Result rawInodeDelete(ID iNodeID);

/**
 * @brief Open an iNode from iNodeID, sets errorcode to indicate error
 * 
 * @param iNodeID ID of iNode to open, contains device ID and iNode block index
 * @return iNode* iNode opened, NULL if error happens
 */
iNode* rawInodeOpen(ID iNodeID);

/**
 * @brief Close an iNode opened, sets errorcode to indicate error
 * 
 * @param iNode iNode to close
 * @return Result Result of the operation
 */
Result rawInodeClose(iNode* iNode);

#endif // __FS_UTIL_H
