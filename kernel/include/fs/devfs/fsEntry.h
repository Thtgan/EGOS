#if !defined(__FS_DEVFS_FSENTRY_H)
#define __FS_DEVFS_FSENTRY_H

#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<kit/types.h>

OldResult devfs_fsEntry_open(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags);

OldResult devfs_fsEntry_create(SuperBlock* superBlock, fsEntryDesc* descOut, ConstCstring name, ConstCstring parentPath, fsEntryType type, DeviceID deviceID, Flags16 flags);

OldResult devfs_fsEntry_buildRootDir(SuperBlock* superBlock);

OldResult devfs_fsEntry_initRootDir(SuperBlock* superBlock);

#endif // __FS_DEVFS_FSENTRY_H
