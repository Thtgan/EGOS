#include<print.h>

#include<algorithms.h>
#include<devices/terminal/terminal.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<string.h>

#define __FLAGS_LEFT_JUSTIFY    FLAG8(0)
#define __FLAGS_EXPLICIT_SIGN   FLAG8(1)
#define __FLAGS_PADDING_SPACE   FLAG8(2)
#define __FLAGS_SPECIFIER       FLAG8(3)
#define __FLAGS_LEADING_ZERO    FLAG8(4)
#define __FLAGS_SIGNED          FLAG8(5)
#define __FLAGS_LOWERCASE       FLAG8(6)

#define __IS_DIGIT(__CH)        ('0' <= (__CH) && (__CH) <= '9')

static int __handlePrintf(void (*charHandler)(char ch), const char* format, va_list args);

/**
 * @brief Read flags from format string
 * 
 * @param format Format string
 * @param flags Pointer to flags
 * @return const char* The format string begin after the strings
 */
static const char* __readFlags(const char* format, Uint8* flags);

/**
 * @brief Read integer from format string, or variable argumrnts
 * 
 * @param format Format string
 * @param writeTo Prointer to integer
 * @param args Variable args
 * @return const char* The format string begin after the integer
 */
static const char* __readInteger(const char* format, int* writeTo, va_list args);

typedef enum {
    LENGTH_MODIFIER_HH,
    LENGTH_MODIFIER_H,
    LENGTH_MODIFIER_NONE,
    LENGTH_MODIFIER_L,
    LENGTH_MODIFIER_LL,
    LENGTH_MODIFIER_J,
    LENGTH_MODIFIER_Z,
    LENGTH_MODIFIER_T,
    LENGTH_MODIFIER_GREAT_L,
} LengthModifier;

/**
 * @brief Read length modifier from format string
 * 
 * @param format Format string
 * @param modifier Pointer to modifier
 * @return const char* The format string begin after the length modifier
 */
static const char* __readLengthModifier(const char* format, LengthModifier* modifier);

/**
 * @brief Print the integer in format
 * 
 * @param num Integer, smaller data type should be expanded to unsined 64
 * @param base Base of the integer, 2 <= base <= 16
 * @param width Width, least number of characters to print
 * @param precision Precision, least number of digits to print
 * @param flags Flags
 * @return int The number of character printed
 */
static int __printInteger(void (*charHandler)(char ch), Uint64 num, int base, int width, int precision, Uint8 flags); //TODO: 64-bit not supported yet

/**
 * @brief Print the character in format
 * 
 * @param ch Character
 * @param width Width, least number of characters to print
 * @param flags Flags
 * @return int The number of character printed
 */
static int __printCharacter(void (*charHandler)(char ch), char ch, int width, Uint8 flags);

/**
 * @brief Print the string in format
 * 
 * @param str String
 * @param width Width, least number of characters to print
 * @param precision Precision, most number of characters in string to print
 * @param flags Flags
 * @return int The number of character printed
 */
static int __printString(void (*charHandler)(char ch), const char* str, int width, int precision, Uint8 flags);

int printf(TerminalLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    int ret = vprintf(level, format, args);

    va_end(args);

    return ret;
}

int sprintf(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);

    int ret = vsprintf(buffer, format, args);

    va_end(args);

    return ret;
}

int vprintf(TerminalLevel level, const char* format, va_list args) {
    Terminal* terminal = getLevelTerminal(level);

    int ret = __handlePrintf(LAMBDA(void, (char ch) {
        terminalOutputChar(terminal, ch);
    }), format, args);

    if (terminal == getCurrentTerminal()) {
        flushDisplay();
    }
    return ret;
}

int vsprintf(char* buffer, const char* format, va_list args) {
    Size len = 0;
    int ret =  __handlePrintf(LAMBDA(void, (char ch) {
        buffer[len++] = ch;
    }), format, args);

    buffer[len] = '\0';

    return ret;
}

int putchar(TerminalLevel level, int ch) {
    Terminal* terminal = getLevelTerminal(level);
    terminalOutputChar(terminal, ch);

    if (terminal == getCurrentTerminal()) {
        flushDisplay();
    }

    return ch;
}

static int __handlePrintf(void (*charHandler)(char ch), const char* format, va_list args) {
    int ret = 0;
    for (; *format != '\0'; ++format) { //Scan the string
        if (*format != '%') {
            charHandler(*format);
            ++ret;
            continue;
        }

        Uint8 flags = 0;
        format = __readFlags(++format, &flags);

        int width = -1, precision = -1; //Width: minimum field length, Precision: minimum number of character to print

        format = __readInteger(format, &width, args);
        if (width < 0) {                //If negative, set to positive and add left justify
            width = -width;
            SET_FLAG_BACK(flags, __FLAGS_LEFT_JUSTIFY);
        }

        if (*format == '.') {
            ++format;
            format = __readInteger(format, &precision, args);
            //If precision is negative, use default precision
        }

        LengthModifier modifier = LENGTH_MODIFIER_NONE;
        format = __readLengthModifier(format, &modifier);

        int base;
        switch (*format) {
            case '%':
                charHandler('%');
                break;
            case 'c':
                switch (modifier) {
                    case LENGTH_MODIFIER_NONE:
                        ret += __printCharacter(charHandler, (char)va_arg(args, int), width, flags);
                        break;
                    case LENGTH_MODIFIER_L:
                        //TODO: Implement wint_t version here
                        break;
                    default:
                        ret += __printCharacter(charHandler, (char)va_arg(args, int), width, flags);
                }
                break;
            case 's':
                switch (modifier) {
                    case LENGTH_MODIFIER_H:
                    case LENGTH_MODIFIER_HH:
                    case LENGTH_MODIFIER_NONE:
                        ret += __printString(charHandler, (const char*)va_arg(args, char*), width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_L:
                        //TODO: Implement wchar_t version here
                        break;
                    default:
                        return -1;
                }
                break;
            case 'd':
            case 'i':
                base = 10;
                SET_FLAG_BACK(flags, __FLAGS_SIGNED);
                switch (modifier) {
                    case LENGTH_MODIFIER_HH:
                    case LENGTH_MODIFIER_H:
                    case LENGTH_MODIFIER_NONE:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, int), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_L:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, long), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_LL:
                    case LENGTH_MODIFIER_GREAT_L:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, long long), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_J:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, Intmax), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_Z:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, Size), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_T:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, Ptrdiff), base, width, precision, flags);
                        break;
                    default:
                        charHandler('e');
                }
                break;
            case 'o':
                base = 8;
                goto uLabel;
            case 'x':
                SET_FLAG_BACK(flags, __FLAGS_LOWERCASE);
            case 'X':
                base = 16;
                goto uLabel;
            case 'u':
                base = 10;
            uLabel:
                switch (modifier) {
                    case LENGTH_MODIFIER_HH:
                    case LENGTH_MODIFIER_H:
                    case LENGTH_MODIFIER_NONE:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, unsigned int), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_L:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, unsigned long), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_LL:
                    case LENGTH_MODIFIER_GREAT_L:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, unsigned long long), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_J:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, Uintmax), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_Z:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, Size), base, width, precision, flags);
                        break;
                    case LENGTH_MODIFIER_T:
                        ret += __printInteger(charHandler, (Uint64)va_arg(args, Ptrdiff), base, width, precision, flags);
                        break;
                    default:
                }
                break;
            case 'f':
            case 'F'://TODO: Add support for float numbers
                break;
            case 'e':
            case 'E':
                break;
            case 'a':
            case 'A':
                break;
            case 'g':
            case 'G':
                break;
            case 'n':
                switch (modifier) {
                    case LENGTH_MODIFIER_HH:
                        *((char*)va_arg(args, char*)) = ret;
                        break;
                    case LENGTH_MODIFIER_H:
                        *((short*)va_arg(args, short*)) = ret;
                        break;
                    case LENGTH_MODIFIER_NONE:
                        *((int*)va_arg(args, int*)) = ret;
                        break;
                    case LENGTH_MODIFIER_L:
                        *((long*)va_arg(args, long*)) = ret;
                        break;
                    case LENGTH_MODIFIER_LL:
                    case LENGTH_MODIFIER_GREAT_L:
                        *((long long*)va_arg(args, long long*)) = ret;
                        break;
                    case LENGTH_MODIFIER_J:
                        *((Uintmax*)va_arg(args, Uintmax*)) = ret;
                        break;
                    case LENGTH_MODIFIER_Z:
                        *((Size*)va_arg(args, Size*)) = ret;
                        break;
                    case LENGTH_MODIFIER_T:
                        *((Ptrdiff*)va_arg(args, Uintptr*)) = ret;
                        break;
                    default:
                }
                break;
            case 'p':
                base = 16;
                SET_FLAG_BACK(flags, __FLAGS_SPECIFIER);
                __printInteger(charHandler, (Uint64)va_arg(args, void*), base, width, precision, flags);
                break;
            default:
                charHandler('%');
                charHandler(*format);
                ret += 2;
        }
    }

    return ret;
}

static const char* __readFlags(const char* format, Uint8* flags) {
    Uint8 f = 0;
    do {
        switch (*(format++)) {
            case '-':
                SET_FLAG_BACK(f, __FLAGS_LEFT_JUSTIFY);
                break;
            case '+':
                SET_FLAG_BACK(f, __FLAGS_EXPLICIT_SIGN);
                break;
            case ' ':
                SET_FLAG_BACK(f, __FLAGS_PADDING_SPACE);
                break;
            case '#':
                SET_FLAG_BACK(f, __FLAGS_SPECIFIER);
                break;
            case '0':
                SET_FLAG_BACK(f, __FLAGS_LEADING_ZERO);
                break;
            default:
                --format;
                *flags = f;
                return format;
        }
    } while (true);

    return NULL;    //Theoreticlly, this is impossible to get called
}

static const char* __readInteger(const char* format, int* writeTo, va_list args) {
    int num = 0;
    if (__IS_DIGIT(*format)) { //Read the width from format string
        num = 0;
        for(; __IS_DIGIT(*format); ++format) {
            num = num * 10 + (*format) - '0';
        }
    } else if (*format == '*') { //Read the width from given data
        ++format;
        num = va_arg(args, int);
    }

    *writeTo = num;
    return format;
}

static const char* __readLengthModifier(const char* format, LengthModifier* modifier) {
    LengthModifier m = LENGTH_MODIFIER_NONE;

    switch (*(format++)) {
        case 'h':
            if (*format == 'h') {
                ++format;
                m = LENGTH_MODIFIER_HH;
            } else {
                m = LENGTH_MODIFIER_H;
            }
            break;
        case 'l':
            if (*format == 'l') {
                ++format;
                m = LENGTH_MODIFIER_LL;
            } else {
                m = LENGTH_MODIFIER_L;
            }
            break;
        case 'j':
            m = LENGTH_MODIFIER_J;
            break;
        case 'z':
            m = LENGTH_MODIFIER_Z;
            break;
        case 't':
            m = LENGTH_MODIFIER_T;
            break;
        case 'L':
            m = LENGTH_MODIFIER_GREAT_L;
            break;
        default:
            --format;
    }

    *modifier = m;
    return format;
}

static const char* _digits = "0123456789ABCDEF";
static char _tmp[64];   //Number temporary buffer

//TODO: BUG: printf("%#02X", 0xAA55) outputs 0XAA55 (should be 0x55)
static int __printInteger(void (*charHandler)(char ch), Uint64 num, int base, int width, int precision, Uint8 flags) {
    if (base < 2 || base > 16)  //If base not available, return
        return -1;              //error

    char sign = '\0';
    int sLen = 0,                                                   //Length of sign or specifier
        ret = 0;                                                    //Return value

    if (TEST_FLAGS(flags, __FLAGS_SPECIFIER)) {                     //If use the specifier or sign, length will be recorded
        if (base == 16) {                                           //Hex
            sLen = 2;                                               //Use specifier 0x
        } else if (base == 8) {                                     //Oct
            sLen = 1;                                               //Use specifier 0
        }
    } else {                                                        //If specifier used(printing hex or oct), sign will not be available
        if (TEST_FLAGS(flags, __FLAGS_SIGNED)) {                    //If not signed (%u), sign will not be available
            sLen = 1;                                               //Default length 1
            if ((Int64)num < 0) {                                 //Negative value
                sign = '-';                                         //Need minus sign
                num = -num;                                         //Use absolute value
            } else if (TEST_FLAGS(flags, __FLAGS_EXPLICIT_SIGN)) {  //Need explicit sign
                sign = '+';
            } else if (TEST_FLAGS(flags, __FLAGS_PADDING_SPACE)) {  //Need padding
                sign = ' ';
            } else {
                sLen = 0;                                           //No sign, change length back
            }
        }
    }

    //Write the number to temp buffer
    Uint8 lowercaseBit = TEST_FLAGS(flags, __FLAGS_LOWERCASE) ? 32 : 0;
    int digitLen = 0;   //Length of digits
    if (num == 0) {
        if (precision != 0) { //If both num and precision is 0, no character will be output
            _tmp[digitLen++] = '0';
        }
    } else {
        for (; num != 0; num /= base) {
            _tmp[digitLen++] = _digits[num % base] | lowercaseBit; //Number character contains lowercase bit, this will not affect the output of 0-9
        }
    }

    int fullLen = 0,                                        //Full length of digits (Specifier and sign not included)
        leadingZeroLen = 0,                                 //Length of leading zero (Contained in fullLen)
        padding = 0;                                        //Length of padding space

    if (precision >= 0) {                                   //Precision not negative, zero flag ignored
        fullLen = max32(digitLen, precision);               //At least as long as precision
    } else if (TEST_FLAGS(flags, __FLAGS_LEADING_ZERO)) {   //No precision, have zero flag
        fullLen = max32(width - sLen, digitLen);            //No padding space, fill field with leading zero
    } else {                                                //No precision, no zero flag
        fullLen = digitLen;                                 //No leading zero
    }

    leadingZeroLen = fullLen - digitLen;
    padding = max32(width - sLen - fullLen, 0);

    ret = padding + sLen + padding; //TODO: Check this code again

    if (TEST_FLAGS_NONE(flags, __FLAGS_LEFT_JUSTIFY)) {
        for (; padding > 0; --padding) {
            charHandler(' ');
        }
    }

    if (sign != '\0') {     //Guaranteed only sign or specifier, impossible to print both
        charHandler(sign);
    }
    if (TEST_FLAGS(flags, __FLAGS_SPECIFIER)) {
        if (base == 8) {
            charHandler('0');
        } else if (base == 16) {
            charHandler('0');
            charHandler('X' | lowercaseBit);
        }
    }

    for (; leadingZeroLen > 0; --leadingZeroLen) {
        charHandler('0');
    }

    for (int i = digitLen - 1; i >= 0; --i) {
        charHandler(_tmp[i]);
    }

    for (; padding > 0; --padding) {    //If left justified, padding should be 0 when entering this loop
        charHandler(' ');
    }

    return ret;
}

static int __printCharacter(void (*charHandler)(char ch), char ch, int width, Uint8 flags) {
    int padding = width - 1;
    if (TEST_FLAGS_NONE(flags, __FLAGS_LEFT_JUSTIFY)) {
        for (; padding > 0; --padding) {
            charHandler(' ');
        }
    }
    charHandler(ch);

    for (; padding > 0; --padding) {
        charHandler(' ');
    }

    return width >= 1 ? width : 1;
}

static int __printString(void (*charHandler)(char ch), const char* str, int width, int precision, Uint8 flags) {
    int strLen = strlen(str), padding = 0;
    if (precision >= 0) {
        strLen = min32(strLen, precision);
    }

    padding = max32(0, width - strLen);

    int ret = padding + strLen;

    if (TEST_FLAGS_NONE(flags, __FLAGS_LEFT_JUSTIFY)) {
        for (; padding > 0; --padding) {
            charHandler(' ');
        }
    }

    for (int i = 0; i < strLen; ++i) {
        charHandler(str[i]);
    }

    for (; padding > 0; --padding) {
        charHandler(' ');
    }

    return ret;
}