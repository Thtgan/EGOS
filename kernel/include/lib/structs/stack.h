#if !defined(__LIB_STRUCTS_STACK_H)
#define __LIB_STRUCTS_STACK_H

typedef struct Stack Stack;

#include<kit/types.h>

typedef struct Stack {
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
 */
void stack_push(Stack* stack, Object obj);

void stack_top(Stack* stack, Object* retPtr);

void stack_pop(Stack* stack);

#endif // __LIB_STRUCTS_STACK_H
