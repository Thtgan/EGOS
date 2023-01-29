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

#define MARK_PRINT(__INFO)                          printf(TERMINAL_LEVEL_DEBUG, MACRO_STR(__INFO) " at %s, line %d\n", __FILE__, __LINE__)

#define ASSERT_SILENT(__EXPRESSION)                 do { if (!(__EXPRESSION)) blowup("Assertion failed at %s, line %d\n", __FILE__, __LINE__); } while(0)

#define ASSERT(__EXPRESSION, __LAST_WORD, ...)      do { if (!(__EXPRESSION)) blowup("Assertion failed at %s, line %d\n" __LAST_WORD "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)

typedef struct {
    union {
        uint8_t bytes[256];
        uint16_t words[128];
        uint32_t dwords[64];
        uint64_t quads[32];
    };
} DataSharer;

#define ACCESS_DATA_SHARER()    extern DataSharer dataSharer

#endif // __DEBUG_H
