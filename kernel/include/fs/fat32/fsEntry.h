#if !defined(__FAT32_FS_ENTRY_H)
#define __FAT32_FS_ENTRY_H

#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/types.h>

#define FAT32_FS_ENTRY_DIRECTORY_ENTRY_SIZE 32

Result fat32_fsEntry_open(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc);

#endif // __FAT32_FS_ENTRY_H
