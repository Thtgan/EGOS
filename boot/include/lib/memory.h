#if !defined(__MEMORY_H)
#define __MEMORY_H

#include<stddef.h>

/**
 * @brief Set byte in a range of memory
 * 
 * @param dest The memory write to
 * @param ch The byte to write
 * @param count The number of byte to write 
 * @return dest
 */
void* memset(void *dest, int ch, size_t count);

#endif // __MEMORY_H
