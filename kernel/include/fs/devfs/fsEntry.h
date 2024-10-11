#if !defined(__FS_DEVFS_FSENTRY_H)
#define __FS_DEVFS_FSENTRY_H

#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<kit/types.h>

Result devfs_fsEntry_open(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags);

Result devfs_fsEntry_create(SuperBlock* superBlock, fsEntryDesc* descOut, fsEntryDescInitArgs* args);

Result devfs_fsEntry_buildRootDir(SuperBlock* superBlock);

Result devfs_fsEntry_initRootDir(SuperBlock* superBlock);

#endif // __FS_DEVFS_FSENTRY_H
