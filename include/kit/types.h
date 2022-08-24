#if !defined(__TYPES_H)
#define __TYPES_H

#include<stddef.h>
#include<stdint.h>

//Get the host pointer containing the node
#define HOST_POINTER(__NODE_PTR, __TYPE, __MEMBER)  ((__TYPE*)(((void*)(__NODE_PTR)) - offsetof(__TYPE, __MEMBER)))

//If this object refers to a struct, use pointer
typedef uint64_t    Object;
#define OBJECT_NULL (Object)NULL

typedef uint64_t    ID;

typedef char*       Cstring;
typedef const char* ConstCstring;

#endif // __TYPES_H
