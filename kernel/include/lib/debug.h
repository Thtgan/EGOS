#if !defined(__DEBUG_H)
#define __DEBUG_H

#include<kit/macro.h>
#include<kit/types.h>
#include<print.h>

__attribute__((noreturn))
/**
 * @brief Blow up the system, print info before that
 * @param format: Format of info
 * @param ...: Values printed by func
 */
void debug_blowup(const Cstring format, ...);

#define DEBUG_MARK_PRINT(__FORMAT, ...)                 printf(TERMINAL_LEVEL_DEBUG, "[" __FILE__ ":%d] " __FORMAT, __LINE__ __VA_OPT__(,) __VA_ARGS__)

#define DEBUG_ASSERT_SILENT(__EXPRESSION)               do { if (!(__EXPRESSION)) debug_blowup("Assertion failed at %s, line %d\n", __FILE__, __LINE__); } while(0)

#define DEBUG_ASSERT(__EXPRESSION, __LAST_WORD, ...)    do { if (!(__EXPRESSION)) debug_blowup("Assertion failed at %s, line %d\n" __LAST_WORD "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)

#endif // __DEBUG_H
