#if !defined(STACK_H)
#define STACK_H

#include<kit/types.h>

typedef struct {
    Object* stack;
    Size maxSize, size;
} Stack;

/**
 * @brief Initialize stack
 * 
 * @param stack Stack struct
 * @param buffer Buffer of stack
 * @param maxSize Maximum capacity of stack
 */
static inline void stack_init(Stack* stack, Object* buffer, Size maxSize) {
    stack->stack = buffer;
    stack->maxSize = maxSize;
    stack->size = 0;
}

/**
 * @brief Is stack empty?
 * 
 * @param stack Stack
 * @return bool True if stack is empty
 */
static inline bool stack_isEmpty(Stack* stack) {
    return stack->size == 0;
}

/**
 * @brief Push an object to stack
 * 
 * @param stack Stack
 * @param obj Object to push
 * @return Result Result of the operation
 */
static inline Result stack_push(Stack* stack, Object obj) {
    if (stack->size == stack->maxSize) {
        return RESULT_FAIL;
    }

    stack->stack[stack->size++] = obj;
    return RESULT_SUCCESS;
}

/**
 * @brief Get the top Object of stack
 * 
 * @param stack Stack
 * @param retPtr Pointer to object
 * @return Result Result of the operation
 */
static inline Result stack_top(Stack* stack, Object* retPtr) {
    if (stack->size == 0) {
        return RESULT_FAIL;
    }

    *retPtr = stack->stack[stack->size - 1];

    return RESULT_SUCCESS;
}

/**
 * @brief Pop the top object of stack
 * 
 * @param stack Stack
 * @return Result Result of the operation
 */
static inline Result stack_pop(Stack* stack) {
    if (stack_isEmpty(stack)) {
        return RESULT_FAIL;
    }

    stack->stack[--stack->size] = OBJECT_NULL;
    
    return RESULT_SUCCESS;
}

#endif // STACK_H
