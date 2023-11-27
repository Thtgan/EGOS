#include<structs/heap.h>

#include<algorithms.h>
#include<kit/types.h>

void heap_initStruct(Heap* heap, Object* objectArray, Size n, CompareFunc compareFunc) {
    heap->objectArray   = objectArray;
    heap->capacity      = n - 1;
    heap->size          = 0;
    heap->compareFunc   = compareFunc;
}

Result heap_push(Heap* heap, Object object) {
    if (heap->size == heap->capacity) {
        return RESULT_FAIL;
    }

    Object* objectArray = heap->objectArray;
    objectArray[++heap->size] = object;
    for (int i = heap->size; i != 1 && heap->compareFunc(objectArray[i], objectArray[i >> 1]) < 0; i >>= 1) {
        swap64(&objectArray[i], &objectArray[i >> 1]);
    }

    return RESULT_SUCCESS;
}

Result heap_top(Heap* heap, Object* object) {
    if (heap->size == 0 || object == NULL) {
        return RESULT_FAIL;
    }

    *object = heap->objectArray[1];

    return RESULT_SUCCESS;
}

Result heap_pop(Heap* heap) {
    if (heap->size == 0) {
        return RESULT_FAIL;
    }

    Object* objectArray = heap->objectArray;
    swap64(&objectArray[1], &objectArray[heap->size--]);
    for (int i = 1, limit = heap->size >> 1; i <= limit; ) {
        int next = i << 1;
        if (next < heap->size && heap->compareFunc(objectArray[next | 1], objectArray[next]) < 0) {
            next |= 1;
        }

        if (heap->compareFunc(objectArray[next], objectArray[i]) >= 0) {
            break;
        }

        swap64(&objectArray[i], &objectArray[next]);
        i = next;
    }

    return RESULT_SUCCESS;
}