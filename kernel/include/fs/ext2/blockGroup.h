#if !defined(__FS_EXT2_BLOCKGROUP_H)
#define __FS_EXT2_BLOCKGROUP_H

typedef struct EXT2blockGroupDescriptor EXT2blockGroupDescriptor;

#include<fs/ext2/ext2.h>
#include<kit/types.h>
#include<debug.h>

typedef struct EXT2blockGroupDescriptor {
    Index32 blockUsageBitmapIndex;
    Index32 inodeUsageBitmapIndex;
    Index32 inodeTableIndex;
    Uint16  freeBlcokNum;
    Uint16  freeInodeNum;
    Uint16  directoryNum;
    Uint8   unused[14];
} __attribute__((packed)) EXT2blockGroupDescriptor;

DEBUG_ASSERT_COMPILE(sizeof(EXT2blockGroupDescriptor) == 32);

Index32 ext2blockGroupDescriptor_allocateBlock(EXT2blockGroupDescriptor* descriptor, EXT2fscore* fscore);

void ext2blockGroupDescriptor_freeBlock(EXT2blockGroupDescriptor* descriptor, EXT2fscore* fscore, Index32 index);

#endif // __FS_EXT2_BLOCKGROUP_H
