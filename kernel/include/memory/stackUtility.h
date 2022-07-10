#if !defined(__STACK_UTILITY)
#define __STACK_UTILITY

#include<stddef.h>
#include<stdint.h>

/**
 * @brief Set up the kernel stack, will set a guard for security
 * 
 * @param mainStackBase RBP in kernel's main
 */
void setupKernelStack(void* mainStackBase);

/**
 * @brief Copy stack, copy ends at the guard
 * 
 * @param oldStackTop RSP of the old stack
 * @param oldStackBase RBP of the old stack
 * @param newStackBottom New stack bottom
 * @return size_t Size of the stack copied
 */
size_t copyCurrentStack(void* oldStackTop, void* oldStackBase, void* newStackBottom);

#endif // __STACK_UTILITY
