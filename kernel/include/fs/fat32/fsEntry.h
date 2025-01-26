#if !defined(__FS_FAT32_FSENTRY_H)
#define __FS_FAT32_FSENTRY_H

#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<kit/types.h>

#define FAT32_FS_ENTRY_DIRECTORY_ENTRY_SIZE 32

void fat32_fsEntry_open(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc, FCNTLopenFlags flags);

void fat32_fsEntry_create(SuperBlock* superBlock, fsEntryDesc* descOut, ConstCstring name, ConstCstring parentPath, fsEntryType type, DeviceID deviceID, Flags16 flags);

#endif // __FS_FAT32_FSENTRY_H
