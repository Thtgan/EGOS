#include<structs/vector.h>

#include<algorithms.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<error.h>

#define __VECTOR_INIT_STORAGE_SIZE  48

void vector_initStruct(Vector* vector) {
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

void vector_resize(Vector* vector, Size newCapacity) {
    Object* newStorage = memory_allocate(newCapacity * sizeof(Object));
    if (newStorage == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    vector->capacity = newCapacity;
    vector->size = algorithms_min64(vector->size, newCapacity);
    memory_memcpy(newStorage, vector->storage, vector->size * sizeof(Object));
    memory_free(vector->storage);
    vector->storage = newStorage;

    return;
    ERROR_FINAL_BEGIN(0);
}

Object vector_get(Vector* vector, Index64 index) {
    if (index >= vector->size) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    return vector->storage[index];
    ERROR_FINAL_BEGIN(0);
    return OBJECT_NULL;
}

void vector_set(Vector* vector, Index64 index, Object item) {
    if (index >= vector->size) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    vector->storage[index] = item;
    return;
    ERROR_FINAL_BEGIN(0);
}

void vector_erease(Vector* vector, Index64 index) {
    if (index >= vector->size) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    memory_memmove(vector->storage + index, vector->storage + index + 1, (vector->capacity - index - 1) * sizeof(Object));
    --vector->size;

    return;
    ERROR_FINAL_BEGIN(0);
}

Object vector_back(Vector* vector) {
    if (vector_isEmpty(vector)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    return vector->storage[vector->size - 1];
    ERROR_FINAL_BEGIN(0);
    return OBJECT_NULL;
}

void vector_push(Vector* vector, Object item) {
    if (vector->size == vector->capacity) {
        ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
    }

    vector_resize(vector, ((vector->capacity * sizeof(Object) + 16) * 2 - 16) / sizeof(Object));
    ERROR_GOTO_IF_ERROR(0);

    vector->storage[vector->size++] = item;
    return;
    ERROR_FINAL_BEGIN(0);
}

void vector_pop(Vector* vector) {
    if (vector_isEmpty(vector)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    vector->storage[vector->size--] = OBJECT_NULL;

    return;
    ERROR_FINAL_BEGIN(0);
}