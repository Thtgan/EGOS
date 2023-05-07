#if !defined(__HARD_DISK_H)
#define __HARD_DISK_H

#define SECTOR_SIZE 512

#include<kit/types.h>

/**
 * @brief Initialize the hard disks
 * 
 * @return Result Result of the operation
 */
Result initHardDisk();

#endif // __HARD_DISK_H
