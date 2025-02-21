#include<fs/directoryEntry.h>

#include<fs/fsEntry.h>
#include<kit/types.h>
#include<cstring.h>

bool directoryEntry_isMatch(DirectoryEntry* entry1, DirectoryEntry* entry2) {
    if (entry1->name != DIRECTORY_ENTRY_NAME_ANY && entry2->name != DIRECTORY_ENTRY_NAME_ANY && cstring_strcmp(entry1->name, entry2->name) != 0) {
        return false;
    }

    if (entry1->type != DIRECTORY_ENTRY_TYPE_ANY && entry2->type != DIRECTORY_ENTRY_TYPE_ANY && entry1->type != entry2->type) {
        return false;
    }

    if (entry1->inodeID != DIRECTORY_ENTRY_INDOE_ID_ANY && entry2->inodeID != DIRECTORY_ENTRY_INDOE_ID_ANY && entry1->inodeID != entry2->inodeID) {
        return false;
    }

    if (entry1->size != DIRECTORY_ENTRY_SIZE_ANY && entry2->size != DIRECTORY_ENTRY_SIZE_ANY && entry1->size != entry2->size) {
        return false;
    }

    if (entry1->mode != DIRECTORY_ENTRY_MODE_ANY && entry2->mode != DIRECTORY_ENTRY_MODE_ANY && entry1->mode != entry2->mode) {
        return false;
    }

    if (entry1->deviceID != DIRECTORY_ENTRY_DEVICE_ID_ANY && entry2->deviceID != DIRECTORY_ENTRY_DEVICE_ID_ANY && entry1->deviceID != entry2->deviceID) {
        return false;
    }

    return true;
}