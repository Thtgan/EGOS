#if !defined(__GDT_INIT_H)
#define __GDT_INIT_H

#include<system/GDT.h>

/**
 * @brief Set up GDT table
 * 
 * @return GDTDesc32* Address of the GDT descriptor
 */
GDTDesc32* setupGDT();

#endif // __GDT_INIT_H
