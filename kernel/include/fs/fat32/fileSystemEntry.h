#if !defined(__FAT32_FILE_SYSTEM_ENTRY_H)
#define __FAT32_FILE_SYSTEM_ENTRY_H

#include<fs/fileSystem.h>
#include<fs/fileSystemEntry.h>
#include<kit/types.h>

#define FAT32_DIRECTORY_ENTRY_SIZE 32

Result FAT32openFileSystemEntry(SuperBlock* superBlock, FileSystemEntry* entry, FileSystemEntryDescriptor* entryDescripotor);

#endif // __FAT32_FILE_SYSTEM_ENTRY_H
