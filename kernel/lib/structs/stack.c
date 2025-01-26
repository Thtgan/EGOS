#include<structs/stack.h>

#include<kit/types.h>
#include<error.h>

void stack_push(Stack* stack, Object obj) {
    if (stack->size == stack->maxSize) {
        ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
    }

    stack->stack[stack->size++] = obj;
    return;
    ERROR_FINAL_BEGIN(0);
}

void stack_top(Stack* stack, Object* retPtr) {
    if (stack_isEmpty(stack)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    *retPtr = stack->stack[stack->size - 1];

    return;
    ERROR_FINAL_BEGIN(0);
}

void stack_pop(Stack* stack) {
    if (stack_isEmpty(stack)) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    stack->stack[--stack->size] = OBJECT_NULL;
    
    return;
    ERROR_FINAL_BEGIN(0);
}