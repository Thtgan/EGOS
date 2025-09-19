#if !defined(__FS_DEVFS_VNODE_H)
#define __FS_DEVFS_VNODE_H

typedef struct DevfsVnode DevfsVnode;
typedef struct DevfsDirectoryEntry DevfsDirectoryEntry;

#include<debug.h>
#include<fs/fsEntry.h>
#include<fs/fsNode.h>
#include<fs/vnode.h>
#include<fs/fscore.h>
#include<kit/types.h>
#include<structs/string.h>

typedef struct DevfsVnode {
    vNode   vnode;
    void*   data;
} DevfsVnode;

typedef struct DevfsDirectoryEntry {
    String          name;
    Index64         mappingIndex;
    Size            size;
    fsEntryType     type;
    Object          pointsTo;
    FSnodeAttribute attribute;
} DevfsDirectoryEntry;

void devfsDirectoryEntry_initStruct(DevfsDirectoryEntry* entry, ConstCstring name, fsEntryType type, Index64 mappingIndex, Object pointsTo, FSnodeAttribute* attribute);

vNodeOperations* devfs_vNode_getOperations();

static inline void* devfs_vNode_getDataPointer(DevfsVnode* node, Index64 index) {
    return node->data + index;
}

#endif // __FS_DEVFS_VNODE_H
