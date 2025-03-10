#if !defined(__LIB_STRUCTS_LOOPARRAY_H)
#define __LIB_STRUCTS_LOOPARRAY_H

typedef struct LoopArray LoopArray;

#include<kit/types.h>

typedef struct LoopArray {
    Size size;
    Size capacity;
    Index64 loopBegin;
    Object* data;
} LoopArray;

void loopArray_initStruct(LoopArray* array, Size capacity);

void loopArray_clearStruct(LoopArray* array);

static inline bool loopArray_isEmpty(LoopArray* array) {
    return array->size == 0;
}

void loopArray_pushBack(LoopArray* array, Object val);

Object loopArray_popBack(LoopArray* array);

Object loopArray_get(LoopArray* array, Index64 index);

Object loopArray_set(LoopArray* array, Index64 index, Object val);

Object loopArray_remove(LoopArray* array, Index64 index);

void loopArray_resize(LoopArray* array, Size newCapacity);

#endif // __LIB_STRUCTS_LOOPARRAY_H
