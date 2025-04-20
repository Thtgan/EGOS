#include<multitask/context.h>

#include<memory/paging.h>
#include<print.h>

__attribute__((naked))
void context_switch(Context* from, Context* to) {
    //Switch the page table, place it here to prevent stack corruption from function call
    asm volatile(
        "mov %%rsp, 8(%0);"
        "mov 8(%1), %%rsp;"
        "push %%rbx;"
        "lea __switch_return(%%rip), %%rbx;"
        "mov %%rbx, 0(%0);"
        "pop %%rbx;"
        "pushq 0(%1);"
        "retq;"
        "__switch_return:"
        "retq;"
        : "=D"(from)
        : "S"(to)
        : "memory"
    );
}
