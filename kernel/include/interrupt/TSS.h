#if !defined(__INTERRUPT_TSS_H)
#define __INTERRUPT_TSS_H

#include<kit/types.h>

/**
 * @brief Initialize TSS
 * 
 * @return OldResult OldResult of the operation
 */
OldResult tss_init();

#endif // __INTERRUPT_TSS_H
