#if !defined(__DIRECTORY_H)
#define __DIRECTORY_H

#include<fs/inode.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<returnValue.h>

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
    ReturnValue (*addEntry)(Directory* directory, iNode* entryInode, ConstCstring name);
    ReturnValue (*removeEntry)(Directory* directory, Index64 entryIndex);
    Index64 (*lookupEntry)(Directory* directory, ConstCstring name, iNodeType type);
    ReturnValue (*readEntry)(Directory* directory, DirectoryEntry* entry, Index64 entryIndex);
};

static inline ReturnValue directoryAddEntry(Directory* directory, iNode* entryInode, ConstCstring name) {
    return directory->operations->addEntry(directory, entryInode, name);
}

static inline ReturnValue directoryRemoveEntry(Directory* directory, Index64 entryIndex) {
    return directory->operations->removeEntry(directory, entryIndex);
}

static inline Index64 directoryLookupEntry(Directory* directory, ConstCstring name, iNodeType type) {
    return directory->operations->lookupEntry(directory, name, type);
}

static inline ReturnValue directoryReadEntry(Directory* directory, DirectoryEntry* entry, Index64 entryIndex) {
    return directory->operations->readEntry(directory, entry, entryIndex);
}

#endif // __DIRECTORY_H
