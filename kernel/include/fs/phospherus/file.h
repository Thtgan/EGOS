#if !defined(__PHOSPHERUS_FILE_H)
#define __PHOSPHERUS_FILE_H

#include<fs/phospherus/allocator.h>
#include<fs/phospherus/inode.h>
#include<stdbool.h>

typedef struct {
    iNodeDesc* inode;
    size_t pointer;
} File;

/**
 * @brief Create new file
 * 
 * @param allocator Block allocator of the device
 * @return block_index_t Index to the inode of the file, return PHOSPHERUS_NULL is create failed
 */
block_index_t phospherus_createFile(Phospherus_Allocator* allocator);

/**
 * @brief Delete file, make sure file is closed before deleting
 * 
 * @param allocator Block allocator of the device
 * @param inodeBlock Index to the inode of the file
 * @return Is the file deleted successfully
 */
bool phospherus_deleteFile(Phospherus_Allocator* allocator, block_index_t inodeBlock);

/**
 * @brief Open file
 * 
 * @param allocator Block allocator of the device
 * @param inodeBlock Index to the inode of the file
 * @return File* File opened, NULL if open failed
 */
File* phospherus_openFile(Phospherus_Allocator* allocator, block_index_t inodeBlock);

/**
 * @brief Close a opened file, cannot close same file twice
 * 
 * @param file File to close
 */
void phospherus_closeFile(File* file);

/**
 * @brief Get the opened file's current pointer, return PHOSPHERUS_NULL if pointer is at the end of file
 * 
 * @param file Opened file
 * @return size_t File pointer PHOSPHERUS_NULL if pointer is at the end of file
 */
size_t phospherus_getFilePointer(File* file);

/**
 * @brief Seek file pointer
 * 
 * @param file Opened file
 * @param pointer Pointer to seek to
 */
void phospherus_seekFile(File* file, size_t pointer);

/**
 * @brief Get size of file
 * 
 * @param file Opened file
 * @return size_t Size of the file
 */
size_t phospherus_getFileSize(File* file);

/**
 * @brief Read data from the file begin from the pointer, and set the pointer to the beginning of the following data
 * 
 * @param file File opened
 * @param buffer Buffer to write to
 * @param size Size of data to read
 * @return size_t File pointer after read
 */
size_t phospherus_readFile(File* file, void* buffer, size_t size);

/**
 * @brief Write data to the file begin from the pointer, and set the pointer to the beginning of the following data, will overwrite data
 * 
 * @param file File opened
 * @param buffer Buffer to read from
 * @param size Size of data to write
 * @return size_t File pointer after write
 */
size_t phospherus_writeFile(File* file, const void* buffer, size_t size);

/**
 * @brief Truncate the file
 * 
 * @param file File opened
 * @param truncateAt Truncate file from
 * @return Is the truncate succeeded
 */
bool phospherus_truncateFile(File* file, size_t truncateAt);

/**
 * @brief Get the file write protection
 * 
 * @param file Opened file
 * @return Is file write protected
 */
bool phospherus_getFileWriteProtection(File* file);

/**
 * @brief Set the file write protection
 * 
 * @param file Opened file
 * @param writeProrection Set file protection or not
 */
void phospherus_setFileWriteProtection(File* file, bool writeProrection);

#endif // __PHOSPHERUS_FILE_H
