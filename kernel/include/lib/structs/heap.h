#if !defined(__LIB_STRUCTS_HEAP_H)
#define __LIB_STRUCTS_HEAP_H

typedef struct Heap Heap;

#include<kit/types.h>

typedef Int64 (*CompareFunc)(Object o1, Object o2);

typedef struct Heap {
    Object*     objectArray;
    Size        capacity;
    Size        size;
    CompareFunc compareFunc;
} Heap;

void heap_initStruct(Heap* heap, Object* objectArray, Size n, CompareFunc compareFunc);

OldResult heap_push(Heap* heap, Object object);

OldResult heap_top(Heap* heap, Object* object);

OldResult heap_pop(Heap* heap);

#endif // __LIB_STRUCTS_HEAP_H
