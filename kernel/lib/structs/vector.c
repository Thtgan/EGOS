#include<structs/vector.h>

#include<algorithms.h>
#include<kit/types.h>
#include<memory/memory.h>

#define __VECTOR_INIT_STORAGE_SIZE  48

void vector_init(Vector* vector) {
    vector->size = 0;
    vector->capacity = __VECTOR_INIT_STORAGE_SIZE / sizeof(Object);
    vector->storage = memory_allocate(__VECTOR_INIT_STORAGE_SIZE);
    memory_memset(vector->storage, OBJECT_NULL, __VECTOR_INIT_STORAGE_SIZE);
}

void vector_clearStruct(Vector* vector) {
    memory_free(vector->storage);
}

bool vector_isEmpty(Vector* vector) {
    return vector->size == 0;
}

void vector_clear(Vector* vector) {
    vector->size = 0;
    memory_memset(vector->storage, OBJECT_NULL, vector->capacity * sizeof(Object));
}

Result vector_resize(Vector* vector, Size newCapacity) {
    Object* newStorage = memory_allocate(newCapacity * sizeof(Object));
    if (newStorage == NULL) {
        return RESULT_FAIL;
    }

    vector->capacity = newCapacity;
    vector->size = algorithms_min64(vector->size, newCapacity);
    memory_memcpy(newStorage, vector->storage, vector->size * sizeof(Object));
    memory_free(vector->storage);
    vector->storage = newStorage;

    return RESULT_SUCCESS;
}

Result vector_get(Vector* vector, Index64 index, Object* retPtr) {
    if (index >= vector->size) {
        return RESULT_FAIL;
    }

    *retPtr = vector->storage[index];
    return RESULT_SUCCESS;
}

Result vector_set(Vector* vector, Index64 index, Object item) {
    if (index >= vector->size) {
        return RESULT_FAIL;
    }

    vector->storage[index] = item;
    return RESULT_SUCCESS;
}

Result vector_erease(Vector* vector, Index64 index) {
    if (index >= vector->size) {
        return RESULT_FAIL;
    }

    memory_memmove(vector->storage + index, vector->storage + index + 1, (vector->capacity - index - 1) * sizeof(Object));
    return RESULT_SUCCESS;
}

Result vector_back(Vector* vector, Object* retPtr) {
    if (vector_isEmpty(vector)) {
        return RESULT_FAIL;
    }

    *retPtr = vector->storage[vector->size - 1];
    return RESULT_SUCCESS;
}

Result vector_push(Vector* vector, Object item) {
    if (vector->size == vector->capacity && vector_resize(vector, ((vector->capacity * sizeof(Object) + 16) * 2 - 16) / sizeof(Object)) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    vector->storage[vector->size++] = item;
    return RESULT_SUCCESS;
}

Result vector_pop(Vector* vector) {
    if (vector_isEmpty(vector)) {
        return RESULT_FAIL;
    }

    vector->storage[vector->size--] = OBJECT_NULL;
    return RESULT_SUCCESS;
}