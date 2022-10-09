#if !defined(__MACRO_H)
#define __MACRO_H

#define MACRO_EVAL(__X) __X

#define MACRO_EXPRESSION(__X) (__X)

#define MACRO_STR(__X) #__X

#define MACRO_CONCENTRATE2(__A, __B) __A ## __B

#define MACRO_CONCENTRATE3(__A, __B, __C) __A ## __B ## __C

#define MACRO_CONCENTRATE4(__A, __B, __C, __D) __A ## __B ## __C ## __D

#define MACRO_CALL(MACRO, ...) MACRO_EVAL(MACRO(__VA_ARGS__))

#endif // __MACRO_H