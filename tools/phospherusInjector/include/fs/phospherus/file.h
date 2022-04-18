#if !defined(__PHOSPHERUS_FILE_H)
#define __PHOSPHERUS_FILE_H

#include<fs/phospherus/allocator.h>
#include<fs/phospherus/inode.h>
#include<stdbool.h>

typedef struct {
    iNodeDesc* inode;
    size_t pointer;
} File;

block_index_t phospherus_createFile(Phospherus_Allocator* allocator);

bool phospherus_deleteFile(Phospherus_Allocator* allocator, block_index_t inodeBlock);

File* phospherus_openFile(Phospherus_Allocator* allocator, block_index_t inodeBlock);

void phospherus_closeFile(File* file);

size_t phospherus_getFilePointer(File* file);

void phospherus_seekFile(File* file, size_t pointer);

size_t phospherus_getFileSize(File* file);

size_t phospherus_readFile(File* file, void* buffer, size_t size);

size_t phospherus_writeFile(File* file, const void* buffer, size_t size);

bool phospherus_truncateFile(File* file, size_t truncateAt);

bool phospherus_getFileWriteProtection(File* file);

void phospherus_setFileWriteProtection(File* file, bool writeProrection);

#endif // __PHOSPHERUS_FILE_H
