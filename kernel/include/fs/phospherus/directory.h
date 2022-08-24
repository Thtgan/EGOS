#if !defined(__PHOSPHERUS_DIRECTORY_H)
#define __PHOSPHERUS_DIRECTORY_H

#include<devices/block/blockDevice.h>
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

/**
 * @brief Create new directory
 * 
 * @param allocator Block allocator of the device
 * @return block_index_t Index to the inode of the directory, return PHOSPHERUS_NULL is create failed
 */
block_index_t phospherus_createDirectory(Phospherus_Allocator* allocator);

/**
 * @brief Create a root directory
 * 
 * @param allocator Block allocator of the device
 * @return block_index_t Index to the inode of the directory, return PHOSPHERUS_NULL is create failed
 */
block_index_t phospherus_createRootDirectory(Phospherus_Allocator* allocator);

/**
 * @brief Delete a directory, make sure directory is closed before deleting
 * 
 * @param allocator Block allocator of the device
 * @param inodeIndex Index to the inode of the directory to delete
 * @return Is the directory deleted successfully
 */
bool phospherus_deleteDirectory(Phospherus_Allocator* allocator, block_index_t inodeIndex);

/**
 * @brief Open directory
 * 
 * @param allocator Block allocator of the device
 * @param inodeIndex Index to the inode of the directory
 * @return Phospherus_Directory* Directory opened, NULL if open failed
 */
Phospherus_Directory* phospherus_openDirectory(Phospherus_Allocator* allocator, block_index_t inodeIndex);

/**
 * @brief Close a opened directory, cannot close same directory twice
 * 
 * @param directory Directory to close
 */
void phospherus_closeDirectory(Phospherus_Directory* directory);



/**
 * @brief Get number of entries in the directory
 * 
 * @param directory Directory opened
 * @return size_t Number of entries in the directory
 */
size_t phospherus_getDirectoryEntryNum(Phospherus_Directory* directory);

/**
 * @brief Search directory entry with name
 * 
 * @param directory Directory opened
 * @param name Name of the entry
 * @param isDirectory Is entry a directory
 * @return size_t Index of the entry in the directory, PHOSPHERUS_NULL if entry not found
 */
size_t phospherus_directorySearchEntry(Phospherus_Directory* directory, const char* name, bool isDirectory);

/**
 * @brief Add entry to the directory, dupliceted name not allowed, unless the entry type is different
 * 
 * @param directory Directory opened
 * @param inodeIndex inode block index to the entry, can be a file or directory
 * @param name Name of the entry, duplicated not allowed
 * @param isDirectory Is new entry a directory
 * @return Is the entry added successfully
 */
bool phospherus_directoryAddEntry(Phospherus_Directory* directory, block_index_t inodeIndex, const char* name, bool isDirectory);

/**
 * @brief Delete the directory entry
 * 
 * @param directory Directory opened
 * @param entryIndex Entry index in the directory
 */
void phospherus_deleteDirectoryEntry(Phospherus_Directory* directory, size_t entryIndex);



/**
 * @brief Get name of the directory entry
 * 
 * @param directory Directory opened
 * @param entryIndex Entry index in the directory, be sure the index is legal
 * @return const char* Name of the entry
 */
const char* phospherus_getDirectoryEntryName(Phospherus_Directory* directory, size_t entryIndex);

/**
 * @brief Set name of the directory entry, be sure the index is legal
 * 
 * @param directory Directory opened
 * @param entryIndex Entry index in the directory, be sure the index is legal
 * @param name New name of the entry
 * @return Is the new name set successfully
 */
bool phospherus_setDirectoryEntryName(Phospherus_Directory* directory, size_t entryIndex, const char* name);

/**
 * @brief Get the index of inode block the entry refers to
 * 
 * @param directory Directory opened
 * @param entryIndex Entry index in the directory, be sure the index is legal
 * @return block_index_t Index of inode block the entry refers to
 */
block_index_t phospherus_getDirectoryEntryInodeIndex(Phospherus_Directory* directory, size_t entryIndex);

/**
 * @brief Check if the entry refers to a directory
 * 
 * @param directory Directory opened
 * @param entryIndex Entry index in the directory, be sure the index is legal
 * @return Is the entry refers to a directory
 */
bool phospherus_checkDirectoryEntryIsDirectory(Phospherus_Directory* directory, size_t entryIndex);

/**
 * @brief Check is the directory is root
 * 
 * @param directory Opened directory
 * @return Is the directory is root
 */
bool phospherus_checkIsRootDirectory(Phospherus_Directory* directory);

#endif // __PHOSPHERUS_DIRECTORY_H
