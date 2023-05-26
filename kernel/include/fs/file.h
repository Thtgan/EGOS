#if !defined(__FILE_H)
#define __FILE_H

#include<fs/inode.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>

#define FILE_FLAG_RW_MASK       MASK_RANGE(8, 0, 2)
#define FILE_FLAG_READ_ONLY     VAL_LEFT_SHIFT(1, 0)
#define FILE_FLAG_WRITE_ONLY    VAL_LEFT_SHIFT(2, 0)
#define FILE_FLAG_READ_WRITE    VAL_LEFT_SHIFT(3, 0)

STRUCT_PRE_DEFINE(File);

typedef struct {
    /**
     * @brief Seek file pointer to position, sets errorcode to indicate error
     * 
     * @param file File
     * @param seekTo Seek to position
     * @return Index64 File pointer after seek, INVALID_INDEX if failed
     */
    Index64 (*seek)(File* file, Size seekTo);

    /**
     * @brief Read from file, sets errorcode to indicate error
     * 
     * @param file File
     * @param buffer Buffer to write to
     * @param n Num of byte(s) to read
     * @return Result Result of the operation
     */
    Result (*read)(File* file, void* buffer, Size n);

    /**
     * @brief Write to file, sets errorcode to indicate error
     * 
     * @param file File
     * @param buffer Buffer to read from
     * @param n Num of byte(s) to write
     * @return Result Result of the operation
     */
    Result (*write)(File* file, const void* buffer, Size n);
} FileOperations;

STRUCT_PRIVATE_DEFINE(File) {
    iNode* iNode;
    Index64 pointer;
    FileOperations operations;
    Flags8 flags;
};

/**
 * @brief Packed function of file operation, sets errorcode to indicate error
 * 
 * @param file File
 * @param seekTo Seek to position
 * @return Index64 File pointer after seek, INVALID_INDEX if failed
 */
static inline Index64 rawFileSeek(File* file, Size seekTo) {
    if (file->operations.seek == NULL) {
        return INVALID_INDEX;
    }
    return file->operations.seek(file, seekTo);
} 

/**
 * @brief Packed function of file operation, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to write to
 * @param n Num of byte(s) to read
 * @return Result Result of the operation
 */
static inline Result rawFileRead(File* file, void* buffer, Size n) {
    if (file->operations.read == NULL) {
        return RESULT_FAIL;
    }
    return file->operations.read(file, buffer, n);
} 

/**
 * @brief Packed function of file operation, sets errorcode to indicate error
 * 
 * @param file File
 * @param buffer Buffer to read from
 * @param n Num of byte(s) to write
 * @return Result Result of the operation
 */
static inline Result rawFileWrite(File* file, const void* buffer, Size n) {
    if (file->operations.write == NULL) {
        return RESULT_FAIL;
    }
    return file->operations.write(file, buffer, n);
} 

#endif // __FILE_H
