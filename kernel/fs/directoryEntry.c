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

    if (entry1->mode != DIRECTORY_ENTRY_MODE_ANY && entry2->mode != DIRECTORY_ENTRY_MODE_ANY && entry1->mode != entry2->mode) {
        return false;
    }

    if (entry1->vnodeID != DIRECTORY_ENTRY_VNDOE_ID_ANY && entry2->vnodeID != DIRECTORY_ENTRY_VNDOE_ID_ANY && entry1->vnodeID != entry2->vnodeID) {
        return false;
    }

    if (entry1->size != DIRECTORY_ENTRY_SIZE_ANY && entry2->size != DIRECTORY_ENTRY_SIZE_ANY && entry1->size != entry2->size) {
        return false;
    }

    if (entry1->pointsTo != DIRECTORY_ENTRY_POINTS_TO_ANY && entry2->pointsTo != DIRECTORY_ENTRY_POINTS_TO_ANY && entry1->pointsTo != entry2->pointsTo) {
        return false;
    }

    return true;
}

bool directoryEntry_isDetailed(DirectoryEntry* entry) {
    return entry->name != DIRECTORY_ENTRY_NAME_ANY && entry->type != DIRECTORY_ENTRY_TYPE_ANY && entry->mode != DIRECTORY_ENTRY_MODE_ANY && entry->vnodeID != DIRECTORY_ENTRY_VNDOE_ID_ANY && entry->size != DIRECTORY_ENTRY_SIZE_ANY && entry->pointsTo != DIRECTORY_ENTRY_POINTS_TO_ANY;
}

bool directoryEntry_checkAdding(DirectoryEntry* entry) {
    return entry->name != DIRECTORY_ENTRY_NAME_ANY && entry->type != DIRECTORY_ENTRY_TYPE_ANY && entry->vnodeID == DIRECTORY_ENTRY_VNDOE_ID_ANY && entry->size == DIRECTORY_ENTRY_SIZE_ANY && !(entry->type == FS_ENTRY_TYPE_DEVICE && entry->pointsTo == DIRECTORY_ENTRY_POINTS_TO_ANY);
}