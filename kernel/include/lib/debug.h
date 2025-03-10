#if !defined(__LIB_DEBUG_H)
#define __LIB_DEBUG_H

#include<multitask/context.h>
#include<kit/macro.h>
#include<kit/types.h>
#include<print.h>
#include<structs/singlyLinkedList.h>

__attribute__((noreturn))
/**
 * @brief Blow up the system, print info before that
 * @param format: Format of info
 * @param ...: Values printed by func
 */
void debug_blowup(ConstCstring format, ...);

void debug_dump_registers(Registers* regs);

void debug_dump_memory(void* data, Size n);

void debug_dump_stack(void* rbp, Size maxDepth);

Uintptr debug_getCurrentRIP();  //TODO: IDK where this should go

#define DEBUG_MARK_PRINT(__FORMAT, ...)                 print_printf("[" __FILE__ ":%d] " __FORMAT, __LINE__ __VA_OPT__(,) __VA_ARGS__)

#define DEBUG_ASSERT_SILENT(__EXPRESSION)               do { if (!(__EXPRESSION)) debug_blowup("Assertion failed at %s, line %d\n", __FILE__, __LINE__); } while(0)

#define DEBUG_ASSERT(__EXPRESSION, __LAST_WORD, ...)    do { if (!(__EXPRESSION)) debug_blowup("Assertion failed at %s, line %d\n" __LAST_WORD, __FILE__, __LINE__, ##__VA_ARGS__); } while(0)

#define DEBUG_ASSERT_COMPILE(__EXPRESSION)              typedef char MACRO_CALL(MACRO_CONCENTRATE2, COMPILE_ASSERT_FAILED_, __LINE__)[-!(__EXPRESSION)]

#endif // __LIB_DEBUG_H
