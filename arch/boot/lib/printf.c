#include<lib/printf.h>

#include<kit/bit.h>
#include<lib/io.h>
#include<lib/string.h>
#include<stdarg.h>

#define __VFPRINTF_FLAGS_LEFT_JUSTIFY   BIT_FLAG8(0)
#define __VFPRINTF_FLAGS_EXPLICIT_SIGN  BIT_FLAG8(1)
#define __VFPRINTF_FLAGS_PADDING_SPACE  BIT_FLAG8(2)
#define __VFPRINTF_FLAGS_SPECIFIER      BIT_FLAG8(3)
#define __VFPRINTF_FLAGS_PADDING_ZERO   BIT_FLAG8(4)
#define __VFPRINTF_FLAGS_SIGNED         BIT_FLAG8(5)
#define __VFPRINTF_FLAGS_LOWERCASE      BIT_FLAG8(6)

#define __IS_DIGIT(__CH)                  ('0' <= (__CH) && (__CH) <= '9')

static char* __writeNumber(char* writeTo, unsigned long num, int base, int width, int precision, uint8_t flags) {
    static const char digits[17] = "0123456789ABCDEF";
    char tmp[128];

    if (base < 2 || base > 16)
        return NULL;

    char sign = 0;
    if (BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_SIGNED)) {
        if (num >= 0x80000000) {
            --width;
            sign = '-';
            num = -num;
        }
        else if (BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_EXPLICIT_SIGN)) {
            --width;
            sign = '+';
        }
        else if (BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_PADDING_SPACE)) {
            --width;
            sign = ' ';
        }
    }

    if (BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_SPECIFIER)) {
        if (base == 16)
            width -= 2;
        else if (base == 8)
            width -= 1;
    }

    char lowercaseBit = BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_LOWERCASE) ? 32 : 0;
    int len = 0;
    if (num == 0)
        tmp[len++] = '0';
    else {
        for (; num != 0; num /= base, ++len)
            tmp[len] = digits[num % base] | lowercaseBit;
    }

    if (len > precision)
        precision = len;
    width -= precision;
    
    if (BIT_TEST_FLAGS_NONE(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY | __VFPRINTF_FLAGS_PADDING_ZERO)) {
        for (; width > 0; --width)
            *writeTo++ = ' ';
    }

    if (sign != 0)
        *writeTo++ = sign;
    if (BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_SPECIFIER)) {
        if (base == 8)
            *writeTo++ = '0';
        else if (base == 16)
        {
            *writeTo++ = '0';
            *writeTo++ = 'X' | lowercaseBit;
        }
    }

    char padding = BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_PADDING_ZERO) ? '0' : ' ';
    if (BIT_TEST_FLAGS_NONE(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY)) {
        for (; width > 0; --width)
            *writeTo++ = padding;
    }

    for (; len < precision; --precision)
        *writeTo++ = '0';

    while (--len >= 0)
        *writeTo++ = tmp[len];
    
    for (; width > 0; --width)
        *writeTo++ = ' ';

    return writeTo;
}

//Reference: https://www.cplusplus.com/reference/cstdio/printf/
int vfprintf(char* buffer, const char* format, va_list args)
{
    char* writeTo = NULL;
    for (writeTo = buffer; *format != '\0'; ++format) {
        if (*format != '%') {
            *writeTo++ = *format;
            continue;
        }

        uint8_t flags = 0;

    loop:
        switch (*(++format)) {
        case '-':
            BIT_SET_FLAG(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY);
            goto loop;
        case '+':
            BIT_SET_FLAG(flags, __VFPRINTF_FLAGS_EXPLICIT_SIGN);
            goto loop;
        case ' ':
            BIT_SET_FLAG(flags, __VFPRINTF_FLAGS_PADDING_SPACE);
            goto loop;
        case '#':
            BIT_SET_FLAG(flags, __VFPRINTF_FLAGS_SPECIFIER);
            goto loop;
        case '0':
            BIT_SET_FLAG(flags, __VFPRINTF_FLAGS_PADDING_ZERO);
            goto loop;
        }

        int width = -1;
        if (__IS_DIGIT(*format)) {
            width = 0;
            for(; __IS_DIGIT(*format); ++format)
                width = width * 10 + (*format) - '0';
        }
        else if (*format == '*') {
            ++format;
            width = va_arg(args, int);
            if (width < 0)
            {
                width = -width;
                BIT_SET_FLAG(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY);
            }
        }

        int precision = -1;
        if (*format == '.') {
            ++format;
            if (__IS_DIGIT(*format)) {
                precision = 0;
                for(; __IS_DIGIT(*format); ++format)
                    precision = precision * 10 + (*format) - '0';
            }
            else if (*format == '*') {
                ++format;
                width = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        int length = -1;
        if (*format == 'h' || *format == 'l')
            length = *format++;

        int base = 10;
        switch (*format) {
        case 'd':
        case 'i':
            BIT_SET_FLAG(flags, __VFPRINTF_FLAGS_SIGNED);
        case 'u':
            break;
        case 'o':
            base = 8;
            break;
        case 'x':
            BIT_SET_FLAG(flags, __VFPRINTF_FLAGS_LOWERCASE);
        case 'X':
            base = 16;
            break;
        case 'c':
            if (!BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY)) {
                while (--width)
                    *writeTo++ = ' ';
            }
            *writeTo++ = (char)va_arg(args, int);
            while (--width)
                *writeTo++ = ' ';
            continue;
        case 's':
            const char* str = va_arg(args, char*);
            int len = strnlen(str, precision);

            if (!BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY)) {
                while (len <= --width)
                    *writeTo++ = ' ';
            }
            for (int i = 0; i < len; ++i)
                *writeTo++ = str[i];
            while (len <= --width)
                *writeTo++ = ' ';
            continue;
        case 'p':
            if (width == -1) {
                width = sizeof(void*) << 1;
                BIT_SET_FLAG(flags, __VFPRINTF_FLAGS_PADDING_ZERO);
            }

            writeTo = __writeNumber(writeTo, (unsigned long)va_arg(args, void*), 16, width, precision, flags);
            
            continue;
        case 'n':
            if (length == 'l') {
                long* ptr = va_arg(args, long*);
                *ptr = writeTo - buffer;
            }
            else if (length == 'h') {
                int* ptr = va_arg(args, int*);
                *ptr = writeTo - buffer;
            }
            continue;
        case '%':
            *writeTo++ = '%';
            continue;
        default:
            *writeTo++ = '%';
            if (*format != '\0')
                *writeTo++ = *format;
            else
                --format;
            continue;
        }

        unsigned long num = 0;
        if (length == 'l') {
            num = (unsigned long)va_arg(args, unsigned long);
            if (BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_SIGNED))
                num = (long)num;
        }
        else if (length == 'h') {
            num = (unsigned short)va_arg(args, unsigned int);
            if (BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_SIGNED))
                num = (short)num;
        }
        else {
            num = (unsigned int)va_arg(args, unsigned int);
            if (BIT_TEST_FLAGS(flags, __VFPRINTF_FLAGS_SIGNED))
                num = (int)num;
        }

        writeTo = __writeNumber(writeTo, num, base, width, precision, flags);
    }
    *writeTo = '\0';
    return writeTo - buffer;
}

int printf(const char* format, ...) {
    char buf[1024];

    va_list args;
    va_start(args, format);

    int ret = vfprintf(buf, format, args);

    va_end(args);

    biosPrint(buf);

    return ret;
}