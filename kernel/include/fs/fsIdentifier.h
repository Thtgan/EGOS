#if !defined(__FS_FSIDENTIFIER_H)
#define __FS_FSIDENTIFIER_H

typedef struct fsIdentifier fsIdentifier;

#include<fs/inode.h>
#include<kit/types.h>
#include<structs/string.h>

typedef struct fsIdentifier {
    iNode*      baseInode;
    String      path;
    bool        isDirectory;
} fsIdentifier;

void fsIdentifier_initStruct(fsIdentifier* identifier, iNode* baseInode, ConstCstring path, bool isDirectory);

static inline bool fsIdentifier_isActive(fsIdentifier* identifier) {
    return identifier->baseInode != NULL && string_isAvailable(&identifier->path);
}

void fsIdentifier_clearStruct(fsIdentifier* identifier);

void fsIdentifier_getAbsolutePath(fsIdentifier* identifier, String* pathOut);

#endif // __FS_FSIDENTIFIER_H
