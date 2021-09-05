#if !defined(__MACRO_H)
#define __MACRO_H

#define MACRO_EVAL(__X) __X

#define MACRO_EXPRESSION(__X) (__X)

#define MACRO_STR(__X) #__X

#define MACRO_CONCENTRATE2(__X, __Y) __X ## __Y

#define MACRO_CONCENTRATE3(__X, __Y, __Z) __X ## __Y ## __Z

#define MACRO_CALL(MACRO, ...) MACRO_EVAL(MACRO(__VA_ARGS__))

#endif // __MACRO_H