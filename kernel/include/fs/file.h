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
     * @brief Seek file pointer to position
     * 
     * @param file File
     * @param seekTo Seek to position
     * @return Index64 File pointer after seek, not exceed file size
     */
    Index64 (*seek)(File* file, size_t seekTo);

    /**
     * @brief Read from file
     * 
     * @param file File
     * @param buffer Buffer to write to
     * @param n Num of byte(s) to read
     * @return int 0 if succeeded
     */
    int (*read)(File* file, void* buffer, size_t n);

    /**
     * @brief Write to file
     * 
     * @param file File
     * @param buffer Buffer to read from
     * @param n Num of byte(s) to write
     * @return int 0 if succeeded
     */
    int (*write)(File* file, const void* buffer, size_t n);
};

/**
 * @brief Packed function of file operation
 * 
 * @param file File
 * @param seekTo Seek to position
 * @return Index64 File pointer after seek, not exceed file size
 */
static inline Index64 fileSeek(File* file, size_t seekTo) {
    return file->operations->seek(file, seekTo);
} 

/**
 * @brief Packed function of file operation
 * 
 * @param file File
 * @param buffer Buffer to write to
 * @param n Num of byte(s) to read
 * @return int 0 if succeeded
 */
static inline int fileRead(File* file, void* buffer, size_t n) {
    return file->operations->read(file, buffer, n);
} 

/**
 * @brief Packed function of file operation
 * 
 * @param file File
 * @param buffer Buffer to read from
 * @param n Num of byte(s) to write
 * @return int 0 if succeeded
 */
static inline int fileWrite(File* file, const void* buffer, size_t n) {
    return file->operations->write(file, buffer, n);
} 

#endif // __FILE_H
