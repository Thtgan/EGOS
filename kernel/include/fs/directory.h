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
    /**
     * @brief Add an entry to the end of directory
     * 
     * @param directory Directory
     * @param iNodeID iNode ID
     * @param type Type of iNode
     * @param name Name of entry
     * @return int 0 if succeeded
     */
    int (*addEntry)(Directory* directory, ID iNodeID, iNodeType type, ConstCstring name);

    /**
     * @brief Remove entry from directory
     * 
     * @param directory Directory
     * @param entryIndex Index of entry
     * @return int 0 if succeeded
     */
    int (*removeEntry)(Directory* directory, Index64 entryIndex);

    /**
     * @brief Look up to an entry in a directory
     * 
     * @param directory Directory
     * @param name Name of entry to look up
     * @param type Type of iNode
     * @return Index64 Index of entry, INVALID_INDEX if entry not exist
     */
    Index64 (*lookupEntry)(Directory* directory, ConstCstring name, iNodeType type);

    /**
     * @brief Read an entry from directory
     * 
     * @param directory Directory
     * @param entry Empty directory entry struct
     * @param entryIndex Index of entry
     * @return int 0 if succeeded
     */
    int (*readEntry)(Directory* directory, DirectoryEntry* entry, Index64 entryIndex);
};

/**
 * @brief Packed function of directory operations
 * 
 * @param directory Directory
 * @param iNodeID iNode ID
 * @param type Type of iNode
 * @param name Name of entry
 * @return int 0 if succeeded
 */
static inline int rawDirectoryAddEntry(Directory* directory, ID iNodeID, iNodeType type, ConstCstring name) {
    return directory->operations->addEntry(directory, iNodeID, type, name);
}

/**
 * @brief Packed function of directory operations
 * 
 * @param directory Directory
 * @param entryIndex Index of entry
 * @return int 0 if succeeded
 */
static inline int rawDirectoryRemoveEntry(Directory* directory, Index64 entryIndex) {
    return directory->operations->removeEntry(directory, entryIndex);
}

/**
 * @brief Packed function of directory operations
 * 
 * @param directory Directory
 * @param name Name of entry to look up
 * @param type Type of iNode
 * @return Index64 Index of entry, INVALID_INDEX if entry not exist
 */
static inline Index64 rawDirectoryLookupEntry(Directory* directory, ConstCstring name, iNodeType type) {
    return directory->operations->lookupEntry(directory, name, type);
}

/**
 * @brief Packed function of directory operations
 * 
 * @param directory Directory
 * @param entry Empty directory entry struct
 * @param entryIndex Index of entry
 * @return int 0 if succeeded
 */
static inline int rawDirectoryReadEntry(Directory* directory, DirectoryEntry* entry, Index64 entryIndex) {
    return directory->operations->readEntry(directory, entry, entryIndex);
}

#endif // __DIRECTORY_H
