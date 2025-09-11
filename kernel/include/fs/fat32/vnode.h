#if !defined(__FS_FAT32_VNODE_H)
#define __FS_FAT32_VNODE_H

typedef struct FAT32vnode FAT32vnode;

#include<kit/types.h>
#include<fs/fsEntry.h>
#include<fs/vnode.h>
#include<fs/fscore.h>

typedef struct FAT32vnode {
    vNode vnode;
    Index32 firstCluster;
} FAT32vnode;

void fat32_vNode_init();

vNodeOperations* fat32_vNode_getOperations();

Size fat32_vNode_touchDirectory(vNode* vnode);

#endif // __FS_FAT32_VNODE_H
