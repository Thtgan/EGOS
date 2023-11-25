#if !defined(__FSUTIL_H)
#define __FSUTIL_H

#include<fs/fileSystemEntry.h>
#include<kit/types.h>

Result fileSystemEntryOpen(FileSystemEntry* entry, FileSystemEntryDescriptor* desc, ConstCstring path, FileSystemEntryType type); //TODO: Remove desc argument?

Result fileSystemEntryClose(FileSystemEntry* entry);

typedef FileSystemEntry* Directory; //TODO: Maybe shouldn't use pointer in typdef
typedef FileSystemEntry* File;

Result directoryLookup(Directory directory, FileSystemEntryDescriptor* descriptor, Size* entrySizePtr, FileSystemEntryIdentifier* identifier);

#define FILE_SEEK_BEGIN     0
#define FILE_SEEK_CURRENT   1
#define FILE_SEEK_END       2

/**
 * @brief Seek file pointer to position, sets errorcode to indicate error
 * 
 * @param file File
 * @param offset Offset to the vegin
 * @param begin Begin of the seek
 * @return Result Result of the operation
 */
Result fileSeek(File file, Int64 offset, Uint8 begin);

/**
 * @brief Get current pointer of file
 * 
 * @param file File
 * @return Index64 Pointer position of file
 */
Index64 fileGetPointer(File file);

/**
 * @brief Read from file, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to write to
 * @param n Num of byte(s) to read
 * @return Result Result of the operation
 */
Result fileRead(File file, void* buffer, Size n);

/**
 * @brief Write to file, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to read from
 * @param n Num of byte(s) to write
 * @return Result Result of the operation
 */
Result fileWrite(File file, const void* buffer, Size n);

#endif // __FSUTIL_H
