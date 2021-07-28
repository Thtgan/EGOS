#if !defined(__MACRO_H)
#define __MACRO_H

#define MACRO_EVAL(X) X

#define MACRO_EXPRESSION(X) (X)

#define MACRO_STR(X) #X

#define MACRO_CONCENTRATE(X, Y) X ## Y

#define MACRO_CALL(MACRO, ...) MACRO_EVAL(MACRO(__VA_ARGS__))

#endif // __MACRO_H