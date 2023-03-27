#if !defined(__FILE_H)
#define __FILE_H

#include<fs/inode.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<returnValue.h>

STRUCT_PRE_DEFINE(FileOperations);

typedef struct {
    iNode* iNode;
    Index64 pointer;
    FileOperations* operations;
} File;

STRUCT_PRIVATE_DEFINE(FileOperations) {
    Index64 (*seek)(File* file, size_t seekTo);
    ReturnValue (*read)(File* file, void* buffer, size_t n);
    ReturnValue (*write)(File* file, const void* buffer, size_t n);
};

static inline Index64 fileSeek(File* file, size_t seekTo) {
    return file->operations->seek(file, seekTo);
} 

static inline ReturnValue fileRead(File* file, void* buffer, size_t n) {
    return file->operations->read(file, buffer, n);
} 

static inline ReturnValue fileWrite(File* file, const void* buffer, size_t n) {
    return file->operations->write(file, buffer, n);
} 

#endif // __FILE_H
