#if !defined(__STDARG_H)
#define __STDARG_H

#include<stdint.h>

typedef uint8_t* va_list;

#define __SIZE_ROUND_UP(__TYPE) ((sizeof(__TYPE) + sizeof(uint8_t) - 1) / sizeof(uint8_t) * sizeof(uint8_t))

#define va_start(__v, __l) (__v = (((va_list) &__l) + __SIZE_ROUND_UP(__l)))

#define va_end(__v) //Do nothing

#define va_arg(__v, __l) (__v += __SIZE_ROUND_UP(__l), *((__l *) (__v - __SIZE_ROUND_UP(__l))))

#endif // __STDARG_H
