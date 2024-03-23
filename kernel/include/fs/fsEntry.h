#if !defined(__FS_ENTRY)
#define __FS_ENTRY

#include<cstring.h>
#include<fs/fsStructs.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/string.h>
#include<structs/hashTable.h>

Result fsEntryIdentifier_initStruct(fsEntryIdentifier* identifier, ConstCstring path, fsEntryType type);

Result fsEntryIdentifier_initStructSep(fsEntryIdentifier* identifier, ConstCstring parentPath, ConstCstring name, fsEntryType type);

void fsEntryIdentifier_clearStruct(fsEntryIdentifier* identifier);

Result fsEntryIdentifier_getParent(fsEntryIdentifier* identifier, fsEntryIdentifier* parentIdentifierOut);

Result fsEntryDesc_initStruct(fsEntryDesc* desc, fsEntryDescInitArgs* args);

void fsEntryDesc_clearStruct(fsEntryDesc* desc);

static inline Index64 fsEntry_rawSeek(fsEntry* entry, Size seekTo) {
    return entry->operations->seek(entry, seekTo);
} 

static inline Result fsEntry_rawRead(fsEntry* entry, void* buffer, Size n) {
    return entry->operations->read(entry, buffer, n);
} 

static inline Result fsEntry_rawWrite(fsEntry* entry, const void* buffer, Size n) {
    return entry->operations->write(entry, buffer, n);
}

static inline Result fsEntry_rawResize(fsEntry* entry, Size newSizeInByte) {
    return entry->operations->resize(entry, newSizeInByte);
}

static inline Result fsEntry_rawDirReadEntry(fsEntry* directory, fsEntryDesc* entryDesc, Size* entrySizePtr) {
    return directory->dirOperations->readEntry(directory, entryDesc, entrySizePtr);
}

static inline Result fsEntry_rawDirAddEntry(fsEntry* directory, fsEntryDesc* entryToAdd) {
    return directory->dirOperations->addEntry(directory, entryToAdd);
}

static inline Result fsEntry_rawDirRemoveEntry(fsEntry* directory, fsEntryIdentifier* entryToRemove) {
    return directory->dirOperations->removeEntry(directory, entryToRemove);
}

static inline Result fsEntry_rawDirUpdateEntry(fsEntry* directory, fsEntryIdentifier* oldEntry, fsEntryDesc* newEntry) {
    return directory->dirOperations->updateEntry(directory, oldEntry, newEntry);
}

Result fsEntry_genericOpen(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc);

Result fsEntry_genericClose(SuperBlock* superBlock, fsEntry* entry);

Index64 fsEntry_genericSeek(fsEntry* entry, Index64 seekTo);

Result fsEntry_genericRead(fsEntry* entry, void* buffer, Size n);

Result fsEntry_genericWrite(fsEntry* entry, const void* buffer, Size n);

Result fsEntry_genericResize(fsEntry* entry, Size newSizeInByte);

#endif // __FS_ENTRY
