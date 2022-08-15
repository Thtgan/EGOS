#if !defined(__DIRECT_ACCESS_H)
#define __DIRECT_ACCESS_H

#include<kit/bit.h>
#include<stddef.h>
#include<stdint.h>
#include<system/address.h>
#include<system/systemInfo.h>

/**
 * @brief Do initialization for direct memory access
 */
void initDirectAccess();

#define DIRECT_ACCESS_VIRTUAL_ADDR_BASE (1ull * TB) //1TB, No one will have more than 1TB memory, right?

#define IS_DIRECT_ACCESS_VA(__VA)       ((uint64_t)(__VA) >= DIRECT_ACCESS_VIRTUAL_ADDR_BASE)

#define PA_TO_DIRECT_ACCESS_VA(__PA)    (IS_DIRECT_ACCESS_VA(__PA) ? NULL : ((void*)(__PA) + DIRECT_ACCESS_VIRTUAL_ADDR_BASE))
#define DIRECT_ACCESS_VA_TO_PA(__VA)    (IS_DIRECT_ACCESS_VA(__VA) ? ((void*)(__VA) - DIRECT_ACCESS_VIRTUAL_ADDR_BASE) : NULL)

//Unsafe operation, no bound checking, treating base in address like flag bit, but no if make it fast, use this in performance critcal part but be careful
#define PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(__PA)    ((void*)VAL_XOR((uint64_t)__PA, DIRECT_ACCESS_VIRTUAL_ADDR_BASE))

#endif // __DIRECT_ACCESS_H
