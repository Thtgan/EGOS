#if !defined(__STACK_UTILITY)
#define __STACK_UTILITY

#include<stddef.h>
#include<stdint.h>

void setupKernelStack(void* mainStackBase);

size_t copyCurrentStack(void* oldStackTop, void* oldStackBase, void* newStackBottom);

#endif // __STACK_UTILITY
