#if !defined(__UTIL_H)
#define __UTIL_H

#include<kit/types.h>
#include<kit/util.h>

#define PTR_TO_VALUE(__LENGTH, __PTR)                   (*(MACRO_EVAL(MACRO_CONCENTRATE2(Uint, __LENGTH))*)(__PTR))

#define DIVIDE_ROUND_UP(__X, __Y)                       (((__X) + (__Y) - 1) / (__Y))
#define DIVIDE_ROUND_DOWN(__X, __Y)                     ((__X) / (__Y))

#define ALIGN_UP(__X, __Y)                              (((__X) + (__Y) - 1) / (__Y) * (__Y))
#define ALIGN_DOWN(__X, __Y)                            ((__X) / (__Y) * (__Y))

//Get the host pointer containing the node
#define HOST_POINTER(__NODE_PTR, __TYPE, __MEMBER)      ((__TYPE*)(((void*)(__NODE_PTR)) - offsetof(__TYPE, __MEMBER)))

#define ARRAY_POINTER_TO_INDEX(__ARRAY, __POINTER)      (Ptrdiff)((__POINTER) - (__ARRAY))

#endif // __UTIL_H
