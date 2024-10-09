#if !defined(__FS_FSUTIL_H)
#define __FS_FSUTIL_H

#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<kit/types.h>

#define FSUTIL_FILE_SEEK_BEGIN      0
#define FSUTIL_FILE_SEEK_CURRENT    1
#define FSUTIL_FILE_SEEK_END        2

/**
 * @brief Seek file pointer to position, sets errorcode to indicate error
 * 
 * @param file File
 * @param offset Offset to the begin
 * @param begin Begin of the seek
 * @return Result Result of the operation
 */
Result fsutil_fileSeek(File* file, Int64 offset, Uint8 begin);

/**
 * @brief Get current pointer of file
 * 
 * @param file File
 * @return Index64 Pointer position of file
 */
Index64 fsutil_fileGetPointer(File* file);

/**
 * @brief Read from file, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to write to
 * @param n Num of byte(s) to read
 * @return Result Result of the operation
 */
Result fsutil_fileRead(File* file, void* buffer, Size n);

/**
 * @brief Write to file, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to read from
 * @param n Num of byte(s) to write
 * @return Result Result of the operation
 */
Result fsutil_fileWrite(File* file, const void* buffer, Size n);

Result fsutil_openfsEntry(SuperBlock* superBlock, ConstCstring path, fsEntryType type, fsEntry* entryOut);

Result fsutil_closefsEntry(fsEntry* entry);

Result fsutil_lookupEntryDesc(Directory* directory, fsEntryIdentifier* identifier, fsEntryDesc* descOut, Size* entrySizeOut);

#endif // __FS_FSUTIL_H
