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
    ID iNodeID;
    iNodeType type;
} DirectoryEntry;

STRUCT_PRIVATE_DEFINE(DirectoryOperations) {
    int (*addEntry)(Directory* directory, ID iNodeID, iNodeType type, ConstCstring name);
    int (*removeEntry)(Directory* directory, Index64 entryIndex);
    Index64 (*lookupEntry)(Directory* directory, ConstCstring name, iNodeType type);
    int (*readEntry)(Directory* directory, DirectoryEntry* entry, Index64 entryIndex);
};

static inline int directoryAddEntry(Directory* directory, ID iNodeID, iNodeType type, ConstCstring name) {
    return directory->operations->addEntry(directory, iNodeID, type, name);
}

static inline int directoryRemoveEntry(Directory* directory, Index64 entryIndex) {
    return directory->operations->removeEntry(directory, entryIndex);
}

static inline Index64 directoryLookupEntry(Directory* directory, ConstCstring name, iNodeType type) {
    return directory->operations->lookupEntry(directory, name, type);
}

static inline int directoryReadEntry(Directory* directory, DirectoryEntry* entry, Index64 entryIndex) {
    return directory->operations->readEntry(directory, entry, entryIndex);
}

#endif // __DIRECTORY_H
