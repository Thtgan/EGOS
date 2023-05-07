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

Result vectorResize(Vector* vector, size_t newCapacity) {
    Object* newStorage = kMalloc(newCapacity * sizeof(Object), MEMORY_TYPE_NORMAL);
    if (newStorage == NULL) {
        return RESULT_FAIL;
    }

    vector->capacity = newCapacity;
    vector->size = min64(vector->size, newCapacity);
    memcpy(newStorage, vector->storage, vector->size * sizeof(Object));
    kFree(vector->storage);
    vector->storage = newStorage;

    return RESULT_SUCCESS;
}

Result vectorGet(Vector* vector, Index64 index, Object* retPtr) {
    if (index >= vector->size) {
        return RESULT_FAIL;
    }

    *retPtr = vector->storage[index];
    return RESULT_SUCCESS;
}

Result vectorSet(Vector* vector, Index64 index, Object item) {
    if (index >= vector->size) {
        return RESULT_FAIL;
    }

    vector->storage[index] = item;
    return RESULT_SUCCESS;
}

Result vectorErase(Vector* vector, Index64 index) {
    if (index >= vector->size) {
        return RESULT_FAIL;
    }

    memmove(vector->storage + index, vector->storage + index + 1, (vector->capacity - index - 1) * sizeof(Object));
    return RESULT_SUCCESS;
}

Result vectorBack(Vector* vector, Object* retPtr) {
    if (vectorIsEmpty(vector)) {
        return RESULT_FAIL;
    }

    *retPtr = vector->storage[vector->size - 1];
    return RESULT_SUCCESS;
}

Result vectorPush(Vector* vector, Object item) {
    if (vector->size == vector->capacity && vectorResize(vector, ((vector->capacity * sizeof(Object) + KMALLOC_HEADER_SIZE) * 2 - KMALLOC_HEADER_SIZE) / sizeof(Object)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    vector->storage[vector->size++] = item;
    return RESULT_SUCCESS;
}

Result vectorPop(Vector* vector) {
    if (vectorIsEmpty(vector)) {
        return RESULT_FAIL;
    }

    vector->storage[vector->size--] = OBJECT_NULL;
    return RESULT_SUCCESS;
}