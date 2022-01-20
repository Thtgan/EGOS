#include<printf.h>

#include<io.h>
#include<kit/bit.h>
#include<stdarg.h>
#include<stdint.h>
#include<string.h>

#define __VFPRINTF_FLAGS_LEFT_JUSTIFY   FLAG8(0)    //Justify the printed number to the left
#define __VFPRINTF_FLAGS_EXPLICIT_SIGN  FLAG8(1)    //Force to print the sign of the number
#define __VFPRINTF_FLAGS_PADDING_SPACE  FLAG8(2)    //Use space for padding
#define __VFPRINTF_FLAGS_SPECIFIER      FLAG8(3)    //Print the specifier like 0, 0x
#define __VFPRINTF_FLAGS_PADDING_ZERO   FLAG8(4)    //Use zero for padding
#define __VFPRINTF_FLAGS_SIGNED         FLAG8(5)    //The number to print is signed
#define __VFPRINTF_FLAGS_LOWERCASE      FLAG8(6)    //Print alphabetic digit in lowercase

#define __IS_DIGIT(__CH)                  ('0' <= (__CH) && (__CH) <= '9')

/**
 * @brief Write the number to the buffer with given format
 * 
 * @param writeTo The buffer to write to
 * @param num Number to write
 * @param base Base of the number
 * @param width Width of the line writed to buffer, fill the buffer with space to fit the width
 * @param precision Real length of the writed number, fill the buffer with zero to fit the precision
 * @param flags Flags to declare the detail while writing the number
 * @return The pointer to the buffer after the number written
 */
static char* __writeNumber(char* writeTo, unsigned long num, int base, int width, int precision, uint8_t flags) {
    static const char digits[17] = "0123456789ABCDEF";  //Character of each digit
    char tmp[128];

    if (base < 2 || base > 16)  //If base not available, return
        return NULL;

    char sign = 0;
    //Determine the sign
    if (TEST_FLAGS(flags, __VFPRINTF_FLAGS_SIGNED)) {
        if (num >= 0x80000000) {
            --width;
            sign = '-';
            num = -num;
        }
        else if (TEST_FLAGS(flags, __VFPRINTF_FLAGS_EXPLICIT_SIGN)) {
            --width;
            sign = '+';
        }
        else if (TEST_FLAGS(flags, __VFPRINTF_FLAGS_PADDING_SPACE)) {
            --width;
            sign = ' ';
        }
    }

    //If use the specifier, modify the width
    if (TEST_FLAGS(flags, __VFPRINTF_FLAGS_SPECIFIER)) {
        if (base == 16)
            width -= 2;
        else if (base == 8)
            width -= 1;
    }

    //Write the number to temp buffer
    char lowercaseBit = TEST_FLAGS(flags, __VFPRINTF_FLAGS_LOWERCASE) ? 32 : 0;
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
    
    //If not left justified or padding zero, pad with space
    if (TEST_FLAGS_NONE(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY | __VFPRINTF_FLAGS_PADDING_ZERO)) {
        for (; width > 0; --width)
            *writeTo++ = ' ';
    }

    //Write the sign and specifier
    if (sign != 0)
        *writeTo++ = sign;
    if (TEST_FLAGS(flags, __VFPRINTF_FLAGS_SPECIFIER)) {
        if (base == 8)
            *writeTo++ = '0';
        else if (base == 16)
        {
            *writeTo++ = '0';
            *writeTo++ = 'X' | lowercaseBit;
        }
    }

    char padding = TEST_FLAGS(flags, __VFPRINTF_FLAGS_PADDING_ZERO) ? '0' : ' ';
    //It is impossible to have space between sign/specifier between the number
    if (TEST_FLAGS_NONE(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY)) {
        for (; width > 0; --width)
            *writeTo++ = padding;
    }

    //Write the remaining zero
    for (; len < precision; --precision)
        *writeTo++ = '0';

    //Write the number
    while (--len >= 0)
        *writeTo++ = tmp[len];
    
    //Write the remaining space
    for (; width > 0; --width)
        *writeTo++ = ' ';

    return writeTo;
}

//Reference: https://www.cplusplus.com/reference/cstdio/printf/
int vfprintf(char* buffer, const char* format, va_list args)
{
    char* writeTo = NULL;
    for (writeTo = buffer; *format != '\0'; ++format) { //Scan the string
        if (*format != '%') {
            *writeTo++ = *format;
            continue;
        }

        uint8_t flags = 0;

    loop: //Goto is awful, but useful
        switch (*(++format)) { //Set the flags
        case '-':
            SET_FLAG_BACK(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY);
            goto loop;
        case '+':
            SET_FLAG_BACK(flags, __VFPRINTF_FLAGS_EXPLICIT_SIGN);
            goto loop;
        case ' ':
            SET_FLAG_BACK(flags, __VFPRINTF_FLAGS_PADDING_SPACE);
            goto loop;
        case '#':
            SET_FLAG_BACK(flags, __VFPRINTF_FLAGS_SPECIFIER);
            goto loop;
        case '0':
            SET_FLAG_BACK(flags, __VFPRINTF_FLAGS_PADDING_ZERO);
            goto loop;
        }

        int width = -1;
        if (__IS_DIGIT(*format)) { //Read the width from format string
            width = 0;
            for(; __IS_DIGIT(*format); ++format)
                width = width * 10 + (*format) - '0';
        }
        else if (*format == '*') { //Read the width from given data
            ++format;
            width = va_arg(args, int);
            if (width < 0)
            {
                width = -width;
                SET_FLAG_BACK(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY);
            }
        }

        int precision = -1;
        if (*format == '.') {
            ++format;
            if (__IS_DIGIT(*format)) { //Read the precision from the format
                precision = 0;
                for(; __IS_DIGIT(*format); ++format)
                    precision = precision * 10 + (*format) - '0';
            }
            else if (*format == '*') { //Read the precision from the data
                ++format;
                width = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        int length = -1; //Type of the data
        if (*format == 'h' || *format == 'l')
            length = *format++;

        int base = 10;
        switch (*format) {
        case 'd':
        case 'i':
            SET_FLAG_BACK(flags, __VFPRINTF_FLAGS_SIGNED);
        case 'u':
            break;
        case 'o':
            base = 8;
            break;
        case 'x':
            SET_FLAG_BACK(flags, __VFPRINTF_FLAGS_LOWERCASE);
        case 'X':
            base = 16;
            break;
        case 'c':
            if (width > 0 && !TEST_FLAGS(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY)) {
                while (--width)
                    *writeTo++ = ' ';
            }
            *writeTo++ = (char)va_arg(args, int);

            if (width <= 0)
                continue;

            while (--width)
                *writeTo++ = ' ';
            continue;
        case 's':
            const char* str = va_arg(args, char*);
            int len = strnlen(str, precision);

            if (width > 0 && !TEST_FLAGS(flags, __VFPRINTF_FLAGS_LEFT_JUSTIFY)) {
                while (len <= --width)
                    *writeTo++ = ' ';
            }
            for (int i = 0; i < len; ++i)
                *writeTo++ = str[i];

            if (width <= len)
                continue;

            while (len <= --width)
                *writeTo++ = ' ';
            continue;
        case 'p':
            if (width == -1) {
                width = sizeof(void*) << 1;
                SET_FLAG_BACK(flags, __VFPRINTF_FLAGS_PADDING_ZERO);
            }

            writeTo = __writeNumber(writeTo, (unsigned long)va_arg(args, void*), 16, width, precision, flags);
            
            continue;
        case 'n': //Length of current string
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
            if (TEST_FLAGS(flags, __VFPRINTF_FLAGS_SIGNED))
                num = (long)num;
        }
        else if (length == 'h') {
            num = (unsigned short)va_arg(args, unsigned int);
            if (TEST_FLAGS(flags, __VFPRINTF_FLAGS_SIGNED))
                num = (short)num;
        }
        else {
            num = (unsigned int)va_arg(args, unsigned int);
            if (TEST_FLAGS(flags, __VFPRINTF_FLAGS_SIGNED))
                num = (int)num;
        }

        writeTo = __writeNumber(writeTo, num, base, width, precision, flags); //Write the number
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