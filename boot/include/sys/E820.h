#if !defined(__E820_H)
#define __E820_H

#include<system/memoryArea.h>
#include<system/systemInfo.h>

/**
 * @brief Detect memory areas
 * 
 * @param info Pointer to system information
 * 
 * @return num of detected memory area, -1 if no memory area detected
 */
int detectMemory(SystemInfo* info);

#endif // __E820_H
