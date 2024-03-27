#if !defined(__DEVFS_FS_ENTRY_H)
#define __DEVFS_FS_ENTRY_H

#include<fs/fsStructs.h>
#include<kit/types.h>

Result devfs_fsEntry_open(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc);

Result devfs_fsEntry_buildRootDir(SuperBlock* superBlock);

Result devfs_fsEntry_initRootDir(SuperBlock* superBlock);

#endif // __DEVFS_FS_ENTRY_H
