#if !defined(__INTERRUPT_TSS_H)
#define __INTERRUPT_TSS_H

#include<kit/types.h>

/**
 * @brief Initialize TSS
 * 
 * @return Result Result of the operation
 */
Result tss_init();

#endif // __INTERRUPT_TSS_H
