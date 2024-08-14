#if !defined(__FS_SYSCALL_H)
#define __FS_SYSCALL_H

#include<fs/fsStructs.h>
#include<kit/bit.h>

#define OPEN_FLAG_READ_ONLY     VAL_LEFT_SHIFT(1, 1)
#define OPEN_FLAG_WRITE_ONLY    VAL_LEFT_SHIFT(1, 2)
#define OPEN_FLAG_READ_WRITE    VAL_LEFT_SHIFT(1, 3)

Result fsSyscall_init();

#define FSSYSCALL_STANDARD_OUTPUT_FILE_DESCRIPTOR   0

File* fsSyscall_getStandardOutputFile();

#endif // __FS_SYSCALL_H
