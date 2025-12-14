#if !defined(__KIT_MACRO_H)
#define __KIT_MACRO_H

#define MACRO_STR(__X) #__X

#define MACRO_CONCENTRATE2(__A, __B) __A ## __B

#define MACRO_CONCENTRATE3(__A, __B, __C) __A ## __B ## __C

#define MACRO_CONCENTRATE4(__A, __B, __C, __D) __A ## __B ## __C ## __D

#define MACRO_CALL(__MACRO, ...)        MACRO_EVAL_1(__MACRO(__VA_ARGS__))    //If call macro fails, try next call macro
#define MACRO_CALL_LOW(__MACRO, ...)    MACRO_EVAL_16(__MACRO(__VA_ARGS__))
#define MACRO_CALL_MID(__MACRO, ...)    MACRO_EVAL_64(__MACRO(__VA_ARGS__))
#define MACRO_CALL_HIGH(__MACRO, ...)   MACRO_EVAL_256(__MACRO(__VA_ARGS__))
#define MACRO_CALL_MAX(__MACRO, ...)    MACRO_EVAL_1024(__MACRO(__VA_ARGS__))

#define MACRO_EMPTY()
#define MACRO_DEFER(__ID) __ID MACRO_EMPTY()

#define MACRO_EVAL(...)         MACRO_EVAL_1024(__VA_ARGS__)

#define MACRO_EVAL_1024(...)    MACRO_EVAL_512(MACRO_EVAL_512(__VA_ARGS__))
#define MACRO_EVAL_512(...)     MACRO_EVAL_256(MACRO_EVAL_256(__VA_ARGS__))
#define MACRO_EVAL_256(...)     MACRO_EVAL_128(MACRO_EVAL_128(__VA_ARGS__))
#define MACRO_EVAL_128(...)     MACRO_EVAL_64(MACRO_EVAL_64(__VA_ARGS__))
#define MACRO_EVAL_64(...)      MACRO_EVAL_32(MACRO_EVAL_32(__VA_ARGS__))
#define MACRO_EVAL_32(...)      MACRO_EVAL_16(MACRO_EVAL_16(__VA_ARGS__))
#define MACRO_EVAL_16(...)      MACRO_EVAL_8(MACRO_EVAL_8(__VA_ARGS__))
#define MACRO_EVAL_8(...)       MACRO_EVAL_4(MACRO_EVAL_4(__VA_ARGS__))
#define MACRO_EVAL_4(...)       MACRO_EVAL_2(MACRO_EVAL_2(__VA_ARGS__))
#define MACRO_EVAL_2(...)       MACRO_EVAL_1(MACRO_EVAL_1(__VA_ARGS__))
#define MACRO_EVAL_1(...)       __VA_ARGS__

#define MACRO_FOREACH_CALL(__MACRO, __CONNECTOR, ...) __VA_OPT__(MACRO_EVAL(__MACRO_FOREACH_CALL_HELPER(__MACRO, __CONNECTOR, __VA_ARGS__)))

#define __MACRO_FOREACH_CALL_HELPER(__MACRO, __CONNECTOR, __ARG1, ...) __MACRO(__ARG1) __VA_OPT__(__CONNECTOR MACRO_DEFER(__MACRO_FOREACH_CALL_RECURSIVE)() (__MACRO, __CONNECTOR, __VA_ARGS__))

#define __MACRO_FOREACH_CALL_RECURSIVE()  __MACRO_FOREACH_CALL_HELPER

#define MACRO_COUNT(...) __MACRO_COUNT_IMPL(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define __MACRO_COUNT_IMPL(__0, __1, __2, __3, __4, __5, __6, __7, __8, __9, __10, __11, __12, __13, __14, __15, __16, __17, __18, __19, __20, __21, __22, __23, __24, __25, __26, __27, __28, __29, __30, __31, __32, __33, __34, __35, __36, __37, __38, __39, __40, __41, __42, __43, __44, __45, __46, __47, __48, __49, __50, __51, __52, __53, __54, __55, __56, __57, __58, __59, __60, __61, __62, __63, __N, ...)  __N

#define MACRO_IF_EMPTY(__A, __B, __C) MACRO_CALL(MACRO_CONCENTRATE2, __MACRO_IF_EMPTY_, MACRO_COUNT(__A)) (__B, __C)

#define __MACRO_IF_EMPTY_0(__B, __C)  __C
#define __MACRO_IF_EMPTY_1(__B, __C)  __B

#endif // __KIT_MACRO_H