#if !defined(__FS_EXT2_VNODE_H)
#define __FS_EXT2_VNODE_H

typedef struct EXT2vnode EXT2vnode;

#include<kit/types.h>
#include<fs/vnode.h>

typedef struct EXT2vnode {
    vNode vnode;
} EXT2vnode;

#endif // __FS_EXT2_VNODE_H
