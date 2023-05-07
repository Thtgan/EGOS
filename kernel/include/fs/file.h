#if !defined(__FILE_H)
#define __FILE_H

#include<fs/inode.h>
#include<kit/oop.h>
#include<kit/types.h>

STRUCT_PRE_DEFINE(FileOperations);

typedef struct {
    iNode* iNode;
    Index64 pointer;
    FileOperations* operations;
} File;

STRUCT_PRIVATE_DEFINE(FileOperations) {
    /**
     * @brief Seek file pointer to position, sets errorcode to indicate error
     * 
     * @param file File
     * @param seekTo Seek to position
     * @return Index64 File pointer after seek, INVALID_INDEX if failed
     */
    Index64 (*seek)(File* file, size_t seekTo);

    /**
     * @brief Read from file, sets errorcode to indicate error
     * 
     * @param file File
     * @param buffer Buffer to write to
     * @param n Num of byte(s) to read
     * @return Result Result of the operation
     */
    Result (*read)(File* file, void* buffer, size_t n);

    /**
     * @brief Write to file, sets errorcode to indicate error
     * 
     * @param file File
     * @param buffer Buffer to read from
     * @param n Num of byte(s) to write
     * @return Result Result of the operation
     */
    Result (*write)(File* file, const void* buffer, size_t n);
};

/**
 * @brief Packed function of file operation, sets errorcode to indicate error
 * 
 * @param file File
 * @param seekTo Seek to position
 * @return Index64 File pointer after seek, INVALID_INDEX if failed
 */
static inline Index64 rawFileSeek(File* file, size_t seekTo) {
    return file->operations->seek(file, seekTo);
} 

/**
 * @brief Packed function of file operation, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to write to
 * @param n Num of byte(s) to read
 * @return Result Result of the operation
 */
static inline Result rawFileRead(File* file, void* buffer, size_t n) {
    return file->operations->read(file, buffer, n);
} 

/**
 * @brief Packed function of file operation, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to read from
 * @param n Num of byte(s) to write
 * @return Result Result of the operation
 */
static inline Result rawFileWrite(File* file, const void* buffer, size_t n) {
    return file->operations->write(file, buffer, n);
} 

#endif // __FILE_H
