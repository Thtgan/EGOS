#if !defined(__FAT32_FS_ENTRY_H)
#define __FAT32_FS_ENTRY_H

#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/types.h>

#define FS_ENTRY_FAT32_DIRECTORY_ENTRY_SIZE 32

Result FAT32_fsEntry_open(SuperBlock* superBlock, FSentry* entry, FSentryDesc* desc);

#endif // __FAT32_FS_ENTRY_H
