#if !defined(__FS_FAT32_FSENTRY_H)
#define __FS_FAT32_FSENTRY_H

#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/types.h>

#define FAT32_FS_ENTRY_DIRECTORY_ENTRY_SIZE 32

Result fat32_fsEntry_open(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc);

#endif // __FS_FAT32_FSENTRY_H
