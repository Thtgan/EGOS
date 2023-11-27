#if !defined(__HEAP_H)
#define __HEAP_H

#include<kit/types.h>

typedef Int64 (*CompareFunc)(Object o1, Object o2);

typedef struct {
    Object*     objectArray;
    Size        capacity;
    Size        size;
    CompareFunc compareFunc;
} Heap;

void heap_initStruct(Heap* heap, Object* objectArray, Size n, CompareFunc compareFunc);

Result heap_push(Heap* heap, Object object);

Result heap_top(Heap* heap, Object* object);

Result heap_pop(Heap* heap);

#endif // __HEAP_H
