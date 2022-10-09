#if !defined(__DIRECTORY_H)
#define __DIRECTORY_H

#include<fs/inode.h>
#include<kit/oop.h>
#include<kit/types.h>

STRUCT_PRE_DEFINE(DirectoryOperations);

typedef struct {
    iNode* iNode;
    size_t size;
    void* directoryInMemory;
    DirectoryOperations* operations;
} Directory;

typedef struct {
    ConstCstring name;
    Index64 iNodeIndex;
    iNodeType type;
} DirectoryEntry;

STRUCT_PRIVATE_DEFINE(DirectoryOperations) {
    int (*addEntry)(THIS_ARG_APPEND(Directory, iNode* entryInode, ConstCstring name));
    int (*removeEntry)(THIS_ARG_APPEND(Directory, Index64 entryIndex));
    Index64 (*lookupEntry)(THIS_ARG_APPEND(Directory, ConstCstring name, iNodeType type));
    DirectoryEntry* (*getEntry)(THIS_ARG_APPEND(Directory, Index64 entryIndex));
};

#endif // __DIRECTORY_H
