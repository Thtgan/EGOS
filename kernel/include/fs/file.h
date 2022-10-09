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
    int (*seek)(THIS_ARG_APPEND(File, size_t seekTo));
    int (*read)(THIS_ARG_APPEND(File, void* buffer, size_t n));
    int (*write)(THIS_ARG_APPEND(File, const void* buffer, size_t n));
};

#endif // __FILE_H
