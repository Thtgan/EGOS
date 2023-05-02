#if !defined(__FS_UTIL_H)
#define __FS_UTIL_H

#include<fs/directory.h>
#include<fs/file.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<kernel.h>
#include<kit/types.h>

/**
 * @brief Trace a path and find directory entry corresponded to the path, starts from root directory
 * 
 * @param entry Direct entry struct
 * @param path Path to trace
 * @param type Type of entry
 * @return int 0 if succeeded
 */
int tracePath(DirectoryEntry* entry, ConstCstring path, iNodeType type);

/**
 * @brief Open file from path, starts from root directory
 * 
 * @param path Path to file
 * @return File* File opened, NULL if open failed
 */
//File* fileOpen(ConstCstring path);
int fileOpen(ConstCstring path);

/**
 * @brief Close file opened
 * 
 * @param file File to close
 * @return int 0 if succeeded
 */
//int fileClose(File* file);
int fileClose(int file);

#define FSUTIL_SEEK_BEGIN   0
#define FSUTIL_SEEK_CURRENT 1
#define FSUTIL_SEEK_END     2

/**
 * @brief Seek file pointer to position
 * 
 * @param file File
 * @param offset Offset to the vegin
 * @param begin Begin of the seek
 * @return int 0 if succeeded
 */
int fileSeek(int file, int64_t offset, uint8_t begin);

/**
 * @brief Get current pointer of file
 * 
 * @param file File
 * @return Index64 Pointer position of file
 */
Index64 fileGetPointer(int file);

/**
 * @brief Read from file
 * 
 * @param file File
 * @param buffer Buffer to write to
 * @param n Num of byte(s) to read
 * @return size_t Num of byte(s) read
 */
//size_t fileRead(File* file, void* buffer, size_t n);
size_t fileRead(int file, void* buffer, size_t n);

/**
 * @brief Write to file
 * 
 * @param file File
 * @param buffer Buffer to read from
 * @param n Num of byte(s) to write
 * @return size_t Num of byte(s) writed
 */
//size_t fileWrite(File* file, const void* buffer, size_t n);
size_t fileWrite(int file, const void* buffer, size_t n);

/**
 * @brief Create an entry and insert it into directory, be aware its parent directory must be existed, this function cannot be used to create entry with type INODE_TYPE_DEVICE,
 * try registerVirtualDevice from virtualDevice.h
 * 
 * @param path Path to new entry's parent directory
 * @param name Name of new entry
 * @param iNodeID iNode ID of new entry
 * @param type Type of new entry
 * @return int 0 if succeeded
 */
int createEntry(ConstCstring path, ConstCstring name, ID iNodeID, iNodeType type);

/**
 * @brief Delete an entry
 * 
 * @param path Path to entry to delete
 * @param type Type of entry to delete
 * @return int 0 if succeeded
 */
int deleteEntry(ConstCstring path, iNodeType type);

/**
 * @brief Open a file from iNode
 * 
 * @param iNode iNode contains file
 * @return File* File opened, NULL if failed
 */
File* rawFileOpen(iNode* iNode);

/**
 * @brief Close a file opened
 * 
 * @param file File to close
 * @return int 0 if succeeded
 */
int rawFileClose(File* file);

/**
 * @brief Open a directory from iNode
 * 
 * @param iNode iNode contains directory
 * @return Directory* Directory opened, NULL if failed
 */
Directory* rawDirectoryOpen(iNode* iNode);

/**
 * @brief Close a directory opened
 * 
 * @param directory Directory to close
 * @return int 0 if succeeded
 */
int rawDirectoryClose(Directory* directory);

/**
 * @brief Create an iNode on device, this function cannot be used to create iNode with type INODE_TYPE_DEVICE
 * 
 * @param deviceID Device to create iNode
 * @param type Type of iNode to create
 * @return Index64 Index of block contiains iNode
 */
Index64 rawInodeCreate(ID deviceID, iNodeType type);

/**
 * @brief Delete an iNode on device
 * 
 * @param iNodeID Device where iNode is
 * @return int 0 if succeeded
 */
int rawInodeDelete(ID iNodeID);

/**
 * @brief Open an iNode from iNodeID
 * 
 * @param iNodeID ID of iNode to open, contains device ID and iNode block index
 * @return iNode* iNode opened, NULL if failed
 */
iNode* rawInodeOpen(ID iNodeID);

/**
 * @brief Close an iNode opened
 * 
 * @param iNode iNode to close
 * @return int 0 if succeeded
 */
int rawInodeClose(iNode* iNode);

#endif // __FS_UTIL_H
