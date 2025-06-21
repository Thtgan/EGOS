#include<structs/loopArray.h>

#include<kit/types.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<algorithms.h>
#include<error.h>

static inline Index64 __loopArray_getRealIndex(LoopArray* array, Index64 index) {
    return (array->loopBegin + index) % array->capacity;
}

void loopArray_initStruct(LoopArray* array, Size capacity) {
    array->capacity = capacity;
    array->size = 0;
    array->loopBegin = 0;
    array->data = mm_allocate(capacity * sizeof(Object));
    if (array->data == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memset(array->data, OBJECT_NULL, capacity * sizeof(Object));

    return;
    ERROR_FINAL_BEGIN(0);
}

void loopArray_clearStruct(LoopArray* array) {
    mm_free(array->data);
}

void loopArray_pushBack(LoopArray* array, Object val) {
    if (array->size < array->capacity) {
        Index64 realIndex = __loopArray_getRealIndex(array, array->size);
        array->data[realIndex] = val;
        ++array->size;
    } else {
        array->data[array->loopBegin] = val;
        array->loopBegin = (array->loopBegin + 1) % array->capacity;
    }
}

Object loopArray_popBack(LoopArray* array) {
    if (array->size == 0) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    Index64 realIndex = __loopArray_getRealIndex(array, array->size - 1);
    Object removed = array->data[realIndex];
    array->data[realIndex] = OBJECT_NULL;
    --array->size;
    
    return removed;
    ERROR_FINAL_BEGIN(0);
    return OBJECT_NULL;
}

Object loopArray_get(LoopArray* array, Index64 index) {
    if (index >= array->size) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    Index64 realIndex = __loopArray_getRealIndex(array, index);
    return array->data[realIndex];
    ERROR_FINAL_BEGIN(0);
    return OBJECT_NULL;
}

Object loopArray_set(LoopArray* array, Index64 index, Object val) {
    if (index >= array->size) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    Index64 realIndex = __loopArray_getRealIndex(array, index);
    Object ret = array->data[realIndex];
    array->data[realIndex] = val;
    return ret;
    ERROR_FINAL_BEGIN(0);
    return OBJECT_NULL;
}

Object loopArray_remove(LoopArray* array, Index64 index) {
    if (index >= array->size) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    Index64 realIndex = __loopArray_getRealIndex(array, index);
    Object ret = array->data[realIndex];

    if (realIndex != array->size - 1) { //Not the last one
        bool isRemainingEntrysCrossing = 
            array->loopBegin + index < array->capacity &&       //Is removed entry at the following space of loopBegin?
            array->loopBegin + array->size > array->capacity;   //Is entrys lay across the physical array?
    
        if (isRemainingEntrysCrossing) {
            memory_memmove(array->data + realIndex, array->data + realIndex + 1, (array->capacity - (realIndex + 1)) * sizeof(Object));
            array->data[array->capacity - 1] = array->data[0];
            memory_memmove(array->data, array->data + 1, (array->size + array->loopBegin - array->capacity - 1) * sizeof(Object));
        } else {
            memory_memmove(array->data + realIndex, array->data + realIndex + 1, (array->size - (index + 1)) * sizeof(Object));
        }
    }

    --array->size;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return OBJECT_NULL;
}

void loopArray_resize(LoopArray* array, Size newCapacity) {
    if (newCapacity == 0) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    if (newCapacity == array->capacity) {
        return;
    }

    Object* newData = mm_allocate(newCapacity * sizeof(Object));
    if (newData == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    memory_memset(newData, OBJECT_NULL, newCapacity * sizeof(Object));
    if (newCapacity < array->capacity) {
        if (array->size <= array->capacity - array->loopBegin) {
            memory_memcpy(newData, array->data + array->loopBegin, algorithms_umin64(array->size, newCapacity) * sizeof(Object));
        } else {
            Size tailEntryNum = array->capacity - array->loopBegin;
            Size headEntryNum = array->size - tailEntryNum;

            if (newCapacity > tailEntryNum) {
                memory_memcpy(newData, array->data + array->loopBegin, tailEntryNum * sizeof(Object));
                memory_memcpy(newData + tailEntryNum * sizeof(Object), array->data, (algorithms_umin64(array->size, newCapacity) - tailEntryNum) * sizeof(Object));
            } else {
                memory_memcpy(newData, array->data + array->loopBegin, newCapacity * sizeof(Object));
            }
        }
    } else {
        if (array->size <= array->capacity - array->loopBegin) {
            memory_memcpy(newData, array->data + array->loopBegin, array->size * sizeof(Object));
        } else {
            Size tailEntryNum = array->capacity - array->loopBegin;
            Size headEntryNum = array->size - tailEntryNum;
            memory_memcpy(newData, array->data + array->loopBegin, tailEntryNum * sizeof(Object));
            memory_memcpy(newData + tailEntryNum * sizeof(Object), array->data, headEntryNum * sizeof(Object));
        }
    }

    array->loopBegin = 0;
    array->capacity = newCapacity;

    ERROR_FINAL_BEGIN(0);
    return;
}