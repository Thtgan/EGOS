#include<print.h>

typedef enum __PrintLengthModifier __PrintLengthModifier;
typedef struct __PrintArgs __PrintArgs;

#include<error.h>
#include<devices/terminal/tty.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<algorithms.h>
#include<cstring.h>
#include<debug.h>

#define __PRINT_FLAGS_LEFT_JUSTIFY  FLAG8(0)
#define __PRINT_FLAGS_EXPLICIT_SIGN FLAG8(1)
#define __PRINT_FLAGS_PADDING_SPACE FLAG8(2)
#define __PRINT_FLAGS_SPECIFIER     FLAG8(3)
#define __PRINT_FLAGS_LEADING_ZERO  FLAG8(4)
#define __PRINT_FLAGS_SIGNED        FLAG8(5)
#define __PRINT_FLAGS_LOWERCASE     FLAG8(6)
#define __PRINT_FLAGS_NUMBER        FLAG8(7)

#define __PRINT_IS_DIGIT(__CH)      ('0' <= (__CH) && (__CH) <= '9')

static int __print_handle(Cstring buffer, Size bufferSize, ConstCstring format, va_list* args);

static ConstCstring __print_readFlags(ConstCstring format, Flags8* flagsRet);

static ConstCstring __print_readInteger(ConstCstring format, int* writeTo, va_list* args);

typedef enum __PrintLengthModifier {
    PRINT_LENGTH_MODIFIER_HH,
    PRINT_LENGTH_MODIFIER_H,
    PRINT_LENGTH_MODIFIER_NONE,
    PRINT_LENGTH_MODIFIER_L,
    PRINT_LENGTH_MODIFIER_LL,
    PRINT_LENGTH_MODIFIER_J,
    PRINT_LENGTH_MODIFIER_Z,
    PRINT_LENGTH_MODIFIER_T,
    PRINT_LENGTH_MODIFIER_GREAT_L,
} __PrintLengthModifier;

typedef struct __PrintArgs {
    __PrintLengthModifier modifier;
    int precision;
    int width;
    Flags8 flags;
} __PrintArgs;

static ConstCstring __print_readModifier(ConstCstring format, __PrintLengthModifier* modifierRet);

static int __print_handleCharacter(Cstring output, Size outputN, int ch, __PrintArgs* printArgs);

static int __print_handleString(Cstring output, Size outputN, ConstCstring str, __PrintArgs* printArgs);

static Uint64 __print_takeInteger(__PrintLengthModifier modifier, va_list* args);

static int __print_handleInteger(Cstring output, Size outputN, Uint64 num, int base, __PrintArgs* printArgs);

static void __print_handleCount(__PrintLengthModifier modifier, int ret, va_list* args);

static void __print_processDefaultPrecision(__PrintArgs* printArgs);

static Size __print_calculateIntegerLength(Uint64 num, int base, __PrintArgs* printArgs);

static Size __print_calculateStringLength(ConstCstring str, __PrintArgs* printArgs);

static void __print_formatInteger(Cstring output, Uint64 num, int base, __PrintArgs* printArgs);

static Size __print_calculatePaddedLength(Size inputN, __PrintArgs* printArgs);

static void __print_formatPadding(Cstring output, ConstCstring input, Size inputN, Size fullLength, __PrintArgs* printArgs);

int print_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    int ret = print_vprintf(format, &args);

    va_end(args);

    return ret;
}

int print_debugPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    int ret = print_debugVprintf(format, &args);

    va_end(args);

    return ret;
}

int print_sprintf(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);

    int ret = print_vsprintf(buffer, format, &args);
    buffer[ret] = '\0';

    va_end(args);

    return ret;
}

#define __PRINT_BUFFER_SIZE 1024

int print_vprintf(const char* format, va_list* args) {
    char buffer[__PRINT_BUFFER_SIZE];
    memory_memset(buffer, 0, __PRINT_BUFFER_SIZE);
    int ret = __print_handle(buffer, (Size)__PRINT_BUFFER_SIZE - 1, format, args);

    Teletype* tty = tty_getCurrentTTY();
    teletype_rawWrite(tty, buffer, ret);
    ERROR_CHECKPOINT(); //Print function is not supposed to throw error
    teletype_rawFlush(tty);
    ERROR_CHECKPOINT();

    return ret;
}

int print_debugVprintf(const char* format, va_list* args) {
    Teletype* tty = debug_getTTY();
    if (tty == NULL) {
        return 0;
    }

    char buffer[__PRINT_BUFFER_SIZE];
    memory_memset(buffer, 0, __PRINT_BUFFER_SIZE);
    int ret = __print_handle(buffer, (Size)__PRINT_BUFFER_SIZE - 1, format, args);

    teletype_rawWrite(tty, buffer, ret);
    ERROR_CHECKPOINT(); //Print function is not supposed to throw error
    teletype_rawFlush(tty);
    ERROR_CHECKPOINT();

    return ret;
}

int print_vsprintf(char* buffer, const char* format, va_list* args) {
    return __print_handle(buffer, (Size)-1, format, args);
}

int print_putchar(int ch) {
    Teletype* tty = tty_getCurrentTTY();
    teletype_rawWrite(tty, &ch, 1);
    ERROR_CHECKPOINT(); //Print function is not supposed to throw error
    teletype_rawFlush(tty);
    ERROR_CHECKPOINT();

    return ch;
}

int print_debugPutchar(int ch) {
    Teletype* tty = debug_getTTY();
    if (tty == NULL) {
        return 0;
    }

    teletype_rawWrite(tty, &ch, 1);
    ERROR_CHECKPOINT(); //Print function is not supposed to throw error
    teletype_rawFlush(tty);
    ERROR_CHECKPOINT();

    return ch;
}

static int __print_handle(Cstring buffer, Size bufferSize, ConstCstring format, va_list* args) {
    int ret = 0;
    for (; *format != '\0'; ++format) { //Scan the string
        if (ret >= bufferSize) {
            break;
        }

        if (*format != '%') {
            buffer[ret++] = *format;
            continue;
        }

        Flags8 flags = EMPTY_FLAGS;
        int width = 0, precision = -1;  //-1 means precision not specified
        //TODO: I failed to figure out how printf handles negative width or precision, so I ignored them
        __PrintLengthModifier modifier = PRINT_LENGTH_MODIFIER_NONE;

        format = __print_readFlags(++format, &flags);
        format = __print_readInteger(format, &width, args);
        width = algorithms_max32(-1, width); //Should not be negative actually

        if (*format == '.') {
            ++format;
            format = __print_readInteger(format, &precision, args);
            precision = algorithms_max32(0, precision); //Should not be negative actually
        }

        format = __print_readModifier(format, &modifier);

        __PrintArgs printArgs = {
            .modifier   = modifier,
            .width      = width,
            .precision  = precision,
            .flags      = flags
        };

        Cstring currentBuffer = buffer + ret;
        Size currentBufferSize = bufferSize - ret;
        int base, res = 0;
        switch (*format) {
            case '%':
                *currentBuffer = '%';
                res = 1;
                break;
            case 'c':
                int ch = va_arg(*args, int);
                res = __print_handleCharacter(currentBuffer, currentBufferSize, ch, &printArgs);
                break;
            case 's':
                ConstCstring str = (ConstCstring)va_arg(*args, ConstCstring);
                res = __print_handleString(currentBuffer, currentBufferSize, str, &printArgs);
                break;
            case 'd':
            case 'i':
                base = 10;
                SET_FLAG_BACK(printArgs.flags, __PRINT_FLAGS_SIGNED);
                goto numberHandleLabel;
            case 'o':
                base = 8;
                goto numberHandleLabel;
            case 'x':
                SET_FLAG_BACK(printArgs.flags, __PRINT_FLAGS_LOWERCASE);
            case 'X':
                base = 16;
                goto numberHandleLabel;
            case 'u':
                base = 10;
                goto numberHandleLabel;
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
                __print_handleCount(modifier, ret, args);
                break;
            case 'p':
                base = 16;
                SET_FLAG_BACK(printArgs.flags, __PRINT_FLAGS_SPECIFIER);
numberHandleLabel:
                SET_FLAG_BACK(printArgs.flags, __PRINT_FLAGS_NUMBER);
                Uint64 num = __print_takeInteger(modifier, args);
                res = __print_handleInteger(currentBuffer, currentBufferSize, num, base, &printArgs);
                break;
            default:
                currentBuffer[0] = '%';
                currentBuffer[1] = *format;
                res = 2;
        }

        if (res < 0) {
            return -1;
        }
        ret += res;
    }

    return ret;
}

static ConstCstring __print_readFlags(ConstCstring format, Flags8* flagsRet) {
    Flags8 flags = EMPTY_FLAGS;

    for (bool reading = true; reading; ++format) {
        switch (*format) {
            case '-':
                SET_FLAG_BACK(flags, __PRINT_FLAGS_LEFT_JUSTIFY);
                break;
            case '+':
                SET_FLAG_BACK(flags, __PRINT_FLAGS_EXPLICIT_SIGN);
                break;
            case ' ':
                SET_FLAG_BACK(flags, __PRINT_FLAGS_PADDING_SPACE);
                break;
            case '#':
                SET_FLAG_BACK(flags, __PRINT_FLAGS_SPECIFIER);
                break;
            case '0':
                SET_FLAG_BACK(flags, __PRINT_FLAGS_LEADING_ZERO);
                break;
            default:
                reading = false;
        }
    }

    --format;
    *flagsRet = flags;
    return format;
}

static ConstCstring __print_readInteger(ConstCstring format, int* writeTo, va_list* args) {
    int num = 0;
    if (__PRINT_IS_DIGIT(*format)) { //Read the width from format string
        num = 0;
        for(; __PRINT_IS_DIGIT(*format); ++format) {
            num = num * 10 + (*format) - '0';
        }
    } else if (*format == '*') { //Read the width from given data
        ++format;
        num = va_arg(*args, int);
    }

    *writeTo = num;
    return format;
}

static ConstCstring __print_readModifier(ConstCstring format, __PrintLengthModifier* modifierRet) {
    __PrintLengthModifier m = PRINT_LENGTH_MODIFIER_NONE;

    switch (*(format++)) {
        case 'h':
            if (*format == 'h') {
                ++format;
                m = PRINT_LENGTH_MODIFIER_HH;
            } else {
                m = PRINT_LENGTH_MODIFIER_H;
            }
            break;
        case 'l':
            if (*format == 'l') {
                ++format;
                m = PRINT_LENGTH_MODIFIER_LL;
            } else {
                m = PRINT_LENGTH_MODIFIER_L;
            }
            break;
        case 'j':
            m = PRINT_LENGTH_MODIFIER_J;
            break;
        case 'z':
            m = PRINT_LENGTH_MODIFIER_Z;
            break;
        case 't':
            m = PRINT_LENGTH_MODIFIER_T;
            break;
        case 'L':
            m = PRINT_LENGTH_MODIFIER_GREAT_L;
            break;
        default:
            --format;
    }

    *modifierRet = m;
    return format;
}

static int __print_handleCharacter(Cstring output, Size outputN, int ch, __PrintArgs* printArgs) {
    __print_processDefaultPrecision(printArgs);
    Size fullLen = __print_calculatePaddedLength(1, printArgs);
    if (fullLen > outputN) {
        return -1;
    }

    char tmp[sizeof(int)];

    switch (printArgs->modifier) {
        case PRINT_LENGTH_MODIFIER_L:
            break;  //TODO: Implement wint_t version here
        default:
            tmp[0] = (char)ch;
            __print_formatPadding(output, tmp, 1, fullLen, printArgs);
            break;
    }

    return fullLen;
}

static int __print_handleString(Cstring output, Size outputN, ConstCstring str, __PrintArgs* printArgs) {
    __print_processDefaultPrecision(printArgs);
    Size strLen = __print_calculateStringLength(str, printArgs);
    Size fullLen = __print_calculatePaddedLength(strLen, printArgs);
    if (fullLen > outputN) {
        return -1;
    }

    switch (printArgs->modifier) {
        case PRINT_LENGTH_MODIFIER_H:
        case PRINT_LENGTH_MODIFIER_HH:
        case PRINT_LENGTH_MODIFIER_NONE:
            __print_formatPadding(output, str, strLen, fullLen, printArgs);
            break;
        case PRINT_LENGTH_MODIFIER_L:
            //TODO: Implement wchar_t version here
            break;
        default:
            return -1;
    }

    return fullLen;
}

static Uint64 __print_takeInteger(__PrintLengthModifier modifier, va_list* args) {
    switch (modifier) {
        case PRINT_LENGTH_MODIFIER_HH:
        case PRINT_LENGTH_MODIFIER_H:
        case PRINT_LENGTH_MODIFIER_NONE:
            return (Uint64)va_arg(*args, int);
        case PRINT_LENGTH_MODIFIER_L:
            return (Uint64)va_arg(*args, long);
        case PRINT_LENGTH_MODIFIER_LL:
        case PRINT_LENGTH_MODIFIER_GREAT_L:
            return (Uint64)va_arg(*args, long long);
        case PRINT_LENGTH_MODIFIER_J:
            return (Uint64)va_arg(*args, Intmax);
        case PRINT_LENGTH_MODIFIER_Z:
            return (Uint64)va_arg(*args, Size);
        case PRINT_LENGTH_MODIFIER_T:
            return (Uint64)va_arg(*args, Ptrdiff);
        default:
            return INFINITE;
    }
    return INFINITE;
}

static int __print_handleInteger(Cstring output, Size outputN, Uint64 num, int base, __PrintArgs* printArgs) {
    __print_processDefaultPrecision(printArgs);

    Size numberLen = __print_calculateIntegerLength(num, base, printArgs);
    Size fullLen = __print_calculatePaddedLength(numberLen, printArgs);
    if (fullLen > outputN) {
        return -1;
    }

    char tmp[numberLen];

    __print_formatInteger(tmp, num, base, printArgs);

    __print_formatPadding(output, tmp, numberLen, fullLen, printArgs);
    
    return fullLen;
}

static void __print_handleCount(__PrintLengthModifier modifier, int ret, va_list* args) {
    switch (modifier) {
        case PRINT_LENGTH_MODIFIER_HH:
            *((char*)va_arg(*args, char*)) = ret;
            break;
        case PRINT_LENGTH_MODIFIER_H:
            *((short*)va_arg(*args, short*)) = ret;
            break;
        case PRINT_LENGTH_MODIFIER_NONE:
            *((int*)va_arg(*args, int*)) = ret;
            break;
        case PRINT_LENGTH_MODIFIER_L:
            *((long*)va_arg(*args, long*)) = ret;
            break;
        case PRINT_LENGTH_MODIFIER_LL:
        case PRINT_LENGTH_MODIFIER_GREAT_L:
            *((long long*)va_arg(*args, long long*)) = ret;
            break;
        case PRINT_LENGTH_MODIFIER_J:
            *((Uintmax*)va_arg(*args, Uintmax*)) = ret;
            break;
        case PRINT_LENGTH_MODIFIER_Z:
            *((Size*)va_arg(*args, Size*)) = ret;
            break;
        case PRINT_LENGTH_MODIFIER_T:
            *((Ptrdiff*)va_arg(*args, Uintptr*)) = ret;
            break;
        default:
    }
}

static void __print_processDefaultPrecision(__PrintArgs* printArgs) {
    if (printArgs->precision == -1) {
        printArgs->precision = TEST_FLAGS(printArgs->flags, __PRINT_FLAGS_NUMBER) ? 1 : 0x7FFFFFFF; //1 for only print minimal numbers, 0x7FFFFFFF for print full string
    }
}

static Size __print_calculateIntegerLength(Uint64 num, int base, __PrintArgs* printArgs) {
    DEBUG_ASSERT_SILENT(base == 8 || base == 10 || base == 16);

    int precision = printArgs->precision;
    Flags8 flags = printArgs->flags;
    
    Size ret = 0;
    if (base == 8 || base == 16) {
        if (TEST_FLAGS(flags, __PRINT_FLAGS_SPECIFIER)) {
            if (base == 8) {
                ret = 1;
            } else if (base == 16) {
                ret = 2;
            }
        }
    } else if (TEST_FLAGS(flags, __PRINT_FLAGS_SIGNED)) {
        if ((Int64)num < 0) {
            ret = 1;
            num = -num;
        } else if (TEST_FLAGS(flags, __PRINT_FLAGS_EXPLICIT_SIGN)) {
            ret = 1;
        }
    }

    int actualLen = 0;
    if (num == 0) {
        if (precision != 0) {
            ++actualLen;
        }
    } else {
        for (; num != 0; num /= base) {
            ++actualLen;    //TODO: Bi-search optimization?
        }
    }

    ret += algorithms_max32(precision, actualLen);

    return ret;
}

static Size __print_calculateStringLength(ConstCstring str, __PrintArgs* printArgs) {
    Size ret = cstring_strlen(str);
    ret = algorithms_umin64(ret, printArgs->precision);
    return ret;
}

static ConstCstring _print_digits = "0123456789ABCDEF";
static void __print_formatInteger(Cstring output, Uint64 num, int base, __PrintArgs* printArgs) {
    DEBUG_ASSERT_SILENT(base == 8 || base == 10 || base == 16);

    int precision = printArgs->precision;
    Flags8 flags = printArgs->flags;
    
    int index = 0;

    Uint8 lowercaseBit = TEST_FLAGS(flags, __PRINT_FLAGS_LOWERCASE) ? 32 : 0;
    if (base == 8 || base == 16) {
        if (TEST_FLAGS(flags, __PRINT_FLAGS_SPECIFIER)) {
            if (base == 8) {
                output[index++] = '0';
            } else if (base == 16) {
                output[index++] = '0';
                output[index++] = 'X' | lowercaseBit;
            }
        }
    } else if (TEST_FLAGS(flags, __PRINT_FLAGS_SIGNED)) {               //Sign is not compatible with hex or oct (Forced unsigned)
        if ((Int64)num < 0) {                                           //Negative value
            output[index++] = '-';                                      //Need minus sign
            num = -num;                                                 //Use absolute value
        } else if (TEST_FLAGS(flags, __PRINT_FLAGS_EXPLICIT_SIGN)) {    //Need explicit sign
            output[index++] = '+';
        }
    }

    char tmp[32];
    
    //Generate the actual number
    int actualLen = 0;          //Length of digits
    if (num == 0) {
        if (precision != 0) {   //If both num and precision is 0, no character will be outputed
            tmp[actualLen++] = '0';
        }
    } else {
        for (; num != 0; num /= base) {
            tmp[actualLen++] = _print_digits[num % base] | lowercaseBit;    //Number character contains lowercase bit, this will not affect the output of 0-9
        }
    }

    int leadingZeroNum = precision - actualLen;
    for (int i = 0; i < leadingZeroNum; ++i) {
        output[index++] = '0';
    }

    for (int i = actualLen - 1; i >= 0; --i) {
        output[index++] = tmp[i];
    }
}

static Size __print_calculatePaddedLength(Size inputN, __PrintArgs* printArgs) {
    Size fullLength = inputN; //Length of actual content and padding space
    if (TEST_FLAGS(printArgs->flags, __PRINT_FLAGS_NUMBER)) {
        fullLength = algorithms_umax32(fullLength, printArgs->precision);
    } else {
        fullLength = algorithms_umin32(fullLength, printArgs->precision);
    }
    fullLength = algorithms_umax32(fullLength, printArgs->width);

    return fullLength;
}

static void __print_formatPadding(Cstring output, ConstCstring input, Size inputN, Size fullLength, __PrintArgs* printArgs) {
    int lPadding = algorithms_max32(0, fullLength - inputN), rPadding = 0;
    if (TEST_FLAGS(printArgs->flags, __PRINT_FLAGS_LEFT_JUSTIFY)) {
        algorithms_swap32(&lPadding, &rPadding);
    }
    
    int index = 0;
    while (lPadding--) {
        output[index++] = ' ';
    }

    for (int i = 0; i < inputN; ++i) {
        output[index++] = input[i];
    }

    while (rPadding--) {
        output[index++] = ' ';
    }
}