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
void blowup(const Cstring format, ...);

#define MARK_PRINT(__INFO)      printf(TERMINAL_LEVEL_DEBUG, MACRO_STR(__INFO) " at %s, line %d\n", __FILE__, __LINE__)

#define ASSERT(__EXPRESSION)    do { if (!(__EXPRESSION)) blowup("Assertion failed at %s, line %d\n", __FILE__, __LINE__); } while(0)

#endif // __DEBUG_H
