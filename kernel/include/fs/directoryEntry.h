#if !defined(__FS_DIRECTORYENTRY_H)
#define __FS_DIRECTORYENTRY_H

typedef struct DirectoryEntry DirectoryEntry;

#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<kit/types.h>

typedef struct DirectoryEntry {
#define DIRECTORY_ENTRY_NAME_ANY        NULL
    ConstCstring name;
#define DIRECTORY_ENTRY_TYPE_ANY        ((fsEntryType)-1)
    fsEntryType type;
#define DIRECTORY_ENTRY_VNDOE_ID_ANY    ((ID)-1)
    ID vnodeID;
#define DIRECTORY_ENTRY_SIZE_ANY        ((Size)-1)
    Size size;
#define DIRECTORY_ENTRY_MODE_ANY        ((Uint16)-1)
    Uint16 mode;    //TODO: Not used yet
#define DIRECTORY_ENTRY_DEVICE_ID_ANY   ((ID)-1)
    ID deviceID;
} DirectoryEntry;

bool directoryEntry_isMatch(DirectoryEntry* entry1, DirectoryEntry* entry2, bool isAnyMatchingDirectory);

#endif // __FS_DIRECTORYENTRY_H
