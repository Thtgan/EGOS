#if !defined(__LIB_FORMAT_H)
#define __LIB_FORMAT_H

#include<kit/types.h>

//Reference: https://en.cppreference.com/w/c/io

int format_process(Cstring buffer, Size bufferSize, ConstCstring format, ...);

int format_vProcess(Cstring buffer, Size bufferSize, ConstCstring format, va_list* args);

#endif // __LIB_FORMAT_H
