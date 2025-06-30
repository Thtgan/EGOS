#if !defined(__INIT_H)
#define __INIT_H

#include<kit/types.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<multitask/schedule.h>
#include<multitask/thread.h>
#include<real/simpleAsmLines.h>
#include<system/pageTable.h>
#include<debug.h>
#include<error.h>

#define INIT_BOOT_STACK_SIZE    PAGE_SIZE
#define INIT_BOOT_STACK_MAGIC   0x00121757AC63A61C

void* init_getBootStackBottom();

void init_initKernelStage1();

__attribute__((always_inline))
static inline void init_initKernelStack() {
    void* newStack = mm_allocateDetailed(
        THREAD_DEFAULT_KERNEL_STACK_SIZE,
        NULL,
        DEFAULT_MEMORY_OPERATIONS_TYPE_COW
    ), * oldStack = init_getBootStackBottom();
    DEBUG_ASSERT_SILENT(newStack != NULL);
    DEBUG_ASSERT_SILENT(*(Uintptr*)readRegister_RBP_64() == INIT_BOOT_STACK_MAGIC);
    
    memory_memset(newStack, 0, THREAD_DEFAULT_KERNEL_STACK_SIZE);
    memory_memcpy(newStack + THREAD_DEFAULT_KERNEL_STACK_SIZE - INIT_BOOT_STACK_SIZE, oldStack, INIT_BOOT_STACK_SIZE);
    Uintptr stackDiff = ((Uintptr)newStack + THREAD_DEFAULT_KERNEL_STACK_SIZE) - ((Uintptr)oldStack + INIT_BOOT_STACK_SIZE);
    writeRegister_RSP_64(readRegister_RSP_64() + stackDiff);
    writeRegister_RBP_64(readRegister_RBP_64() + stackDiff);

    DEBUG_ASSERT_SILENT(*(Uintptr*)readRegister_RBP_64() == INIT_BOOT_STACK_MAGIC);

    schedule_setEarlyStackBottom(newStack);
}

void init_initKernelStage2();

#define INIT_KERNEL()   do {    \
    init_initKernelStage1();    \
    ERROR_CHECKPOINT();         \
    init_initKernelStack();     \
    init_initKernelStage2();    \
    ERROR_CHECKPOINT();         \
} while(0)

#endif // __INIT_H
