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

void initHeap(Heap* heap, Object* objectArray, Size n, CompareFunc compareFunc);

Result heapPush(Heap* heap, Object object);

Result heapTop(Heap* heap, Object* object);

Result heapPop(Heap* heap);

#endif // __HEAP_H
