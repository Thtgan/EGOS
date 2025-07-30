#if !defined(__FS_DEVFS_VNODE_H)
#define __FS_DEVFS_VNODE_H

typedef struct DevfsVnode DevfsVnode;
typedef struct DevfsDirectoryEntry DevfsDirectoryEntry;

#include<debug.h>
#include<fs/fsEntry.h>
#include<fs/vnode.h>
#include<fs/fscore.h>
#include<kit/types.h>
#include<structs/string.h>

typedef struct DevfsVnode {
    vNode   vnode;
    void*   data;
} DevfsVnode;

typedef struct DevfsDirectoryEntry {
    String      name;
    Object      pointTo;
    Size        size;
    fsEntryType type;
    ID          vnodeID;
} DevfsDirectoryEntry;

DEBUG_ASSERT_COMPILE(sizeof(DevfsDirectoryEntry) == 64);

void devfsDirectoryEntry_initStruct(DevfsDirectoryEntry* entry, ConstCstring name, fsEntryType type, Object pointTo, FScore* fscore);

vNodeOperations* devfs_vNode_getOperations();

#endif // __FS_DEVFS_VNODE_H
