#if !defined(__STDARG_H)
#define __STDARG_H

typedef __builtin_va_list va_list;

#define va_start(__LIST, __LAST_ARG)    __builtin_va_start(__LIST, __LAST_ARG)
#define va_end(__LIST)                  __builtin_va_end(__LIST)
#define va_arg(__LIST, __TYPE)          __builtin_va_arg(__LIST, __TYPE)
#define va_copy(__DST, __SRC)	        __builtin_va_copy(__DST, __SRC)

#endif // __STDARG_H
