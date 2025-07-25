#if !defined(__FS_FSIDENTIFIER_H)
#define __FS_FSIDENTIFIER_H

typedef struct fsIdentifier fsIdentifier;

#include<fs/vnode.h>
#include<kit/types.h>
#include<structs/string.h>

typedef struct fsIdentifier {
    vNode*      baseVnode;
    String      path;
    bool        isDirectory;
} fsIdentifier;

void fsIdentifier_initStruct(fsIdentifier* identifier, vNode* baseVnode, ConstCstring path, bool isDirectory);

static inline bool fsIdentifier_isActive(fsIdentifier* identifier) {
    return identifier->baseVnode != NULL && string_isAvailable(&identifier->path);
}

void fsIdentifier_clearStruct(fsIdentifier* identifier);

void fsIdentifier_getAbsolutePath(fsIdentifier* identifier, String* pathOut);

#endif // __FS_FSIDENTIFIER_H
