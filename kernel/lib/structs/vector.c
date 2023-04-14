#include<structs/vector.h>

#include<algorithms.h>
#include<kit/types.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>

#define __VECTOR_INIT_STORAGE_SIZE  (64 - KMALLOC_HEADER_SIZE)

void initVector(Vector* vector) {
    vector->size = 0;
    vector->capacity = __VECTOR_INIT_STORAGE_SIZE / sizeof(Object);
    vector->storage = kMalloc(__VECTOR_INIT_STORAGE_SIZE, MEMORY_TYPE_NORMAL);
    memset(vector->storage, OBJECT_NULL, __VECTOR_INIT_STORAGE_SIZE);
}

void releaseVector(Vector* vector) {
    kFree(vector->storage);
}

bool vectorIsEmpty(Vector* vector) {
    return vector->size == 0;
}

void vectorClear(Vector* vector) {
    vector->size = 0;
    memset(vector->storage, OBJECT_NULL, vector->capacity * sizeof(Object));
}

bool vectorResize(Vector* vector, size_t newCapacity) {
    Object* newStorage = kMalloc(newCapacity * sizeof(Object), MEMORY_TYPE_NORMAL);
    if (newStorage == NULL) {
        return false;
    }

    vector->capacity = newCapacity;
    vector->size = min64(vector->size, newCapacity);
    memcpy(newStorage, vector->storage, vector->size * sizeof(Object));
    kFree(vector->storage);
    vector->storage = newStorage;

    return true;
}

bool vectorGet(Vector* vector, Index64 index, Object* retPtr) {
    if (index >= vector->size) {
        return false;
    }

    *retPtr = vector->storage[index];
    return true;
}

bool vectorSet(Vector* vector, Index64 index, Object item) {
    if (index >= vector->size) {
        return false;
    }

    vector->storage[index] = item;
    return true;
}

bool vectorErase(Vector* vector, Index64 index) {
    if (index >= vector->size) {
        return false;
    }

    memmove(vector->storage + index, vector->storage + index + 1, (vector->capacity - index - 1) * sizeof(Object));
    return true;
}

bool vectorBack(Vector* vector, Object* retPtr) {
    if (vectorIsEmpty(vector)) {
        return false;
    }

    *retPtr = vector->storage[vector->size - 1];
    return true;
}

bool vectorPush(Vector* vector, Object item) {
    if (vector->size == vector->capacity && !vectorResize(vector, ((vector->capacity * sizeof(Object) + KMALLOC_HEADER_SIZE) * 2 - KMALLOC_HEADER_SIZE) / sizeof(Object))) {
        return false;
    }

    vector->storage[vector->size++] = item;
    return true;
}

bool vectorPop(Vector* vector) {
    if (vectorIsEmpty(vector)) {
        return false;
    }

    vector->storage[vector->size--] = OBJECT_NULL;
    return true;
}