#if !defined(__STACK_UTILITY)
#define __STACK_UTILITY

#include<stdint.h>

void setupKernelStack(void* kernelStackBottom, void* mainStackBase);

void copyCurrentStack(void* oldStackTop, void* oldStackBase, void* newStackBottom);

#endif // __STACK_UTILITY
