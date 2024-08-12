#include<multitask/context.h>

#include<devices/terminal/terminalSwitch.h>
#include<memory/paging.h>
#include<print.h>

__attribute__((naked))
void context_switch(Context* from, Context* to) {
    //Switch the page table, place it here to prevent stack corruption from function call
    PAGING_SWITCH_TO_TABLE(to->extendedTable);
    asm volatile(
        "mov %%rsp, 16(%0);"
        "mov 16(%1), %%rsp;"
        "push %%rbx;"
        "lea __switch_return(%%rip), %%rbx;"
        "mov %%rbx, 8(%0);"
        "pop %%rbx;"
        "pushq 8(%1);"
        "retq;"
        "__switch_return:"
        "retq;"
        : "=D"(from)
        : "S"(to)
        : "memory"
    );
}
