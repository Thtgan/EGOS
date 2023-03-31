#if !defined(__VECTOR_H)
#define __VECTOR_H

#include<kit/types.h>

typedef struct {
    size_t size;
    size_t capacity;
    Object* storage;
} Vector;

void initVector(Vector* vector);

void destroyVector(Vector* vector);

bool vectorIsEmpty(Vector* vector);

void vectorClear(Vector* vector);

bool vectorResize(Vector* vector, size_t newCapacity);

bool vectorGet(Vector* vector, Index64 index, Object* retPtr);

bool vectorSet(Vector* vector, Index64 index, Object item);

bool vectorErase(Vector* vector, Index64 index);

bool vectorBack(Vector* vector, Object* retPtr);

bool vectorPush(Vector* vector, Object item);

bool vectorPop(Vector* vector);

#endif // __VECTOR_H
