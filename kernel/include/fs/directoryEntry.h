#if !defined(__FS_DIRECTORYENTRY_H)
#define __FS_DIRECTORYENTRY_H

typedef struct DirectoryEntry DirectoryEntry;

// #include<fs/fsEntry.h>
#include<kit/types.h>

typedef struct DirectoryEntry {
#define DIRECTORY_ENTRY_NAME_ANY        NULL
    ConstCstring name;
#define DIRECTORY_ENTRY_TYPE_ANY        ((Uint32)-1)
    Uint32 type;
#define DIRECTORY_ENTRY_MODE_ANY        ((Uint16)-1)
    Uint16 mode;    //TODO: Not used yet
#define DIRECTORY_ENTRY_VNDOE_ID_ANY    ((ID)0x7FFFFFFFFFFFFFFF)    //Differ from INVALID_ID
    ID vnodeID;
#define DIRECTORY_ENTRY_SIZE_ANY        ((Size)0x7FFFFFFFFFFFFFFF)  //Differ from INFINITE
    Size size;
#define DIRECTORY_ENTRY_POINTS_TO_ANY   INVALID_INDEX64
    Index64 pointsTo;
} DirectoryEntry;

bool directoryEntry_isMatch(DirectoryEntry* entry1, DirectoryEntry* entry2);

static inline bool directoryEntry_isDetailed(DirectoryEntry* entry) {
    return entry->name != DIRECTORY_ENTRY_NAME_ANY && entry->type != DIRECTORY_ENTRY_TYPE_ANY && entry->mode != DIRECTORY_ENTRY_MODE_ANY && entry->vnodeID != DIRECTORY_ENTRY_VNDOE_ID_ANY && entry->size != DIRECTORY_ENTRY_SIZE_ANY && entry->pointsTo != DIRECTORY_ENTRY_POINTS_TO_ANY;
}

#endif // __FS_DIRECTORYENTRY_H
