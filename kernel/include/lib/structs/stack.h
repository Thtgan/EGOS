#if !defined(STACK_H)
#define STACK_H

#include<kit/types.h>

typedef struct {
    Object* stack;
    size_t maxSize, size;
} Stack;

static inline void initStack(Stack* stack, Object* buffer, size_t maxSize) {
    stack->stack = buffer;
    stack->maxSize = maxSize;
    stack->size = 0;
}

static inline bool isStackEmpty(Stack* stack) {
    return stack->size == 0;
}

static inline bool stackPush(Stack* stack, Object obj) {
    if (stack->size == stack->maxSize) {
        return false;
    }

    stack->stack[stack->size++] = obj;
    return true;
}

static inline bool stackTop(Stack* stack, Object* retPtr) {
    if (stack->size == 0) {
        return false;
    }

    *retPtr = stack->stack[stack->size - 1];

    return true;
}

static inline bool stackPop(Stack* stack) {
    if (isStackEmpty(stack)) {
        return false;
    }

    stack->stack[--stack->size] = OBJECT_NULL;
    
    return true;
}

#endif // STACK_H
