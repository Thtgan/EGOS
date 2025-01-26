#if !defined(__FS_FSUTIL_H)
#define __FS_FSUTIL_H

#include<fs/fcntl.h>
#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<kit/types.h>

//These functions are ONLY for FS codes

/**
 * @brief Seek file pointer to position, sets errorcode to indicate error
 * 
 * @param file File
 * @param offset Offset to the begin
 * @param begin Begin of the seek
 */
Index64 fsutil_fileSeek(File* file, Int64 offset, Uint8 begin);

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
 */
void fsutil_fileRead(File* file, void* buffer, Size n);

/**
 * @brief Write to file, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to read from
 * @param n Num of byte(s) to write
 */
void fsutil_fileWrite(File* file, const void* buffer, Size n);

void fsutil_openfsEntry(SuperBlock* superBlock, ConstCstring path, fsEntry* entryOut, FCNTLopenFlags flags);

void fsutil_closefsEntry(fsEntry* entry);

void fsutil_lookupEntryDesc(Directory* directory, ConstCstring name, bool isDirectory, fsEntryDesc* descOut, Size* entrySizeOut);

void fsutil_loacateRealIdentifier(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock** superBlockOut, fsEntryIdentifier* identifierOut);

void fsutil_seekLocalFSentryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntryDesc** descOut);

#endif // __FS_FSUTIL_H
