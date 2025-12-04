#include<format.h>

typedef enum __FormatLengthModifier __FormatLengthModifier;
typedef struct __FormatArgs __FormatArgs;

#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<algorithms.h>
#include<cstring.h>
#include<debug.h>

#define __FORMAT_FLAGS_LEFT_JUSTIFY     FLAG8(0)
#define __FORMAT_FLAGS_EXPLICIT_SIGN    FLAG8(1)
#define __FORMAT_FLAGS_PADDING_SPACE    FLAG8(2)
#define __FORMAT_FLAGS_SPECIFIER        FLAG8(3)
#define __FORMAT_FLAGS_LEADING_ZERO     FLAG8(4)
#define __FORMAT_FLAGS_SIGNED           FLAG8(5)
#define __FORMAT_FLAGS_LOWERCASE        FLAG8(6)
#define __FORMAT_FLAGS_NUMBER           FLAG8(7)

#define __FORMAT_IS_DIGIT(__CH)      ('0' <= (__CH) && (__CH) <= '9')

static ConstCstring __format_readFlags(ConstCstring format, Flags8* flagsRet);

static ConstCstring __format_readInteger(ConstCstring format, int* writeTo, va_list* args);

typedef enum __FormatLengthModifier {
    FORMAT_LENGTH_MODIFIER_HH,
    FORMAT_LENGTH_MODIFIER_H,
    FORMAT_LENGTH_MODIFIER_NONE,
    FORMAT_LENGTH_MODIFIER_L,
    FORMAT_LENGTH_MODIFIER_LL,
    FORMAT_LENGTH_MODIFIER_J,
    FORMAT_LENGTH_MODIFIER_Z,
    FORMAT_LENGTH_MODIFIER_T,
    FORMAT_LENGTH_MODIFIER_GREAT_L,
} __FormatLengthModifier;

typedef struct __FormatArgs {
    __FormatLengthModifier modifier;
    int precision;
    int width;
    Flags8 flags;
} __FormatArgs;

static ConstCstring __format_readModifier(ConstCstring format, __FormatLengthModifier* modifierRet);

static int __format_handleCharacter(Cstring output, Size outputN, int ch, __FormatArgs* printArgs);

static int __format_handleString(Cstring output, Size outputN, ConstCstring str, __FormatArgs* printArgs);

static Uint64 __format_takeInteger(__FormatLengthModifier modifier, va_list* args);

static int __format_handleInteger(Cstring output, Size outputN, Uint64 num, int base, __FormatArgs* printArgs);

static void __format_handleCount(__FormatLengthModifier modifier, int ret, va_list* args);

static void __format_processDefaultPrecision(__FormatArgs* printArgs);

static Size __format_calculateIntegerLength(Uint64 num, int base, __FormatArgs* printArgs);

static Size __format_calculateStringLength(ConstCstring str, __FormatArgs* printArgs);

static void __format_formatInteger(Cstring output, Uint64 num, int base, __FormatArgs* printArgs);

static void __format_formatPadding(Cstring output, ConstCstring input, Size inputN, Size fullLength, __FormatArgs* printArgs);

int format_process(Cstring buffer, Size bufferSize, ConstCstring format, ...) {
    va_list args;
    va_start(args, format);

    int ret = format_vProcess(buffer, bufferSize, format, &args);

    va_end(args);

    return ret;
}

int format_vProcess(Cstring buffer, Size bufferSize, ConstCstring format, va_list* args) {
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
        int width = -1, precision = -1; //-1 means not specified
        //TODO: I failed to figure out how printf handles negative width or precision, so I ignored them
        __FormatLengthModifier modifier = FORMAT_LENGTH_MODIFIER_NONE;

        format = __format_readFlags(++format, &flags);
        format = __format_readInteger(format, &width, args);
        width = algorithms_max32(-1, width); //Should not be negative actually

        if (*format == '.') {
            ++format;
            format = __format_readInteger(format, &precision, args);
            precision = algorithms_max32(0, precision); //Should not be negative actually
        }

        format = __format_readModifier(format, &modifier);

        __FormatArgs printArgs = {
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
                res = __format_handleCharacter(currentBuffer, currentBufferSize, ch, &printArgs);
                break;
            case 's':
                ConstCstring str = (ConstCstring)va_arg(*args, ConstCstring);
                res = __format_handleString(currentBuffer, currentBufferSize, str, &printArgs);
                break;
            case 'd':
            case 'i':
                base = 10;
                SET_FLAG_BACK(printArgs.flags, __FORMAT_FLAGS_SIGNED);
                goto numberHandleLabel;
            case 'o':
                base = 8;
                goto numberHandleLabel;
            case 'x':
                SET_FLAG_BACK(printArgs.flags, __FORMAT_FLAGS_LOWERCASE);
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
                __format_handleCount(modifier, ret, args);
                break;
            case 'p':
                base = 16;
                SET_FLAG_BACK(printArgs.flags, __FORMAT_FLAGS_SPECIFIER);
numberHandleLabel:
                SET_FLAG_BACK(printArgs.flags, __FORMAT_FLAGS_NUMBER);
                Uint64 num = __format_takeInteger(modifier, args);
                res = __format_handleInteger(currentBuffer, currentBufferSize, num, base, &printArgs);
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

static ConstCstring __format_readFlags(ConstCstring format, Flags8* flagsRet) {
    Flags8 flags = EMPTY_FLAGS;

    for (bool reading = true; reading; ++format) {
        switch (*format) {
            case '-':
                SET_FLAG_BACK(flags, __FORMAT_FLAGS_LEFT_JUSTIFY);
                break;
            case '+':
                SET_FLAG_BACK(flags, __FORMAT_FLAGS_EXPLICIT_SIGN);
                break;
            case ' ':
                SET_FLAG_BACK(flags, __FORMAT_FLAGS_PADDING_SPACE);
                break;
            case '#':
                SET_FLAG_BACK(flags, __FORMAT_FLAGS_SPECIFIER);
                break;
            case '0':
                SET_FLAG_BACK(flags, __FORMAT_FLAGS_LEADING_ZERO);
                break;
            default:
                reading = false;
        }
    }

    --format;
    *flagsRet = flags;
    return format;
}

static ConstCstring __format_readInteger(ConstCstring format, int* writeTo, va_list* args) {
    int num = 0;
    if (__FORMAT_IS_DIGIT(*format)) { //Read the width from format string
        num = 0;
        for(; __FORMAT_IS_DIGIT(*format); ++format) {
            num = num * 10 + (*format) - '0';
        }
    } else if (*format == '*') { //Read the width from given data
        ++format;
        num = va_arg(*args, int);
    }

    *writeTo = num;
    return format;
}

static ConstCstring __format_readModifier(ConstCstring format, __FormatLengthModifier* modifierRet) {
    __FormatLengthModifier m = FORMAT_LENGTH_MODIFIER_NONE;

    switch (*(format++)) {
        case 'h':
            if (*format == 'h') {
                ++format;
                m = FORMAT_LENGTH_MODIFIER_HH;
            } else {
                m = FORMAT_LENGTH_MODIFIER_H;
            }
            break;
        case 'l':
            if (*format == 'l') {
                ++format;
                m = FORMAT_LENGTH_MODIFIER_LL;
            } else {
                m = FORMAT_LENGTH_MODIFIER_L;
            }
            break;
        case 'j':
            m = FORMAT_LENGTH_MODIFIER_J;
            break;
        case 'z':
            m = FORMAT_LENGTH_MODIFIER_Z;
            break;
        case 't':
            m = FORMAT_LENGTH_MODIFIER_T;
            break;
        case 'L':
            m = FORMAT_LENGTH_MODIFIER_GREAT_L;
            break;
        default:
            --format;
    }

    *modifierRet = m;
    return format;
}

static int __format_handleCharacter(Cstring output, Size outputN, int ch, __FormatArgs* printArgs) {
    __format_processDefaultPrecision(printArgs);
    Size fullLen = algorithms_max32(1, printArgs->width);
    if (fullLen > outputN) {
        return -1;
    }

    char tmp[sizeof(int)];

    switch (printArgs->modifier) {
        case FORMAT_LENGTH_MODIFIER_L:
            break;  //TODO: Implement wint_t version here
        default:
            tmp[0] = (char)ch;
            __format_formatPadding(output, tmp, 1, fullLen, printArgs);
            break;
    }

    return fullLen;
}

static int __format_handleString(Cstring output, Size outputN, ConstCstring str, __FormatArgs* printArgs) {
    __format_processDefaultPrecision(printArgs);

    Size strLen = __format_calculateStringLength(str, printArgs);
    Size fullLen = algorithms_max32(strLen, printArgs->width);
    if (fullLen > outputN) {
        return -1;
    }

    switch (printArgs->modifier) {
        case FORMAT_LENGTH_MODIFIER_H:
        case FORMAT_LENGTH_MODIFIER_HH:
        case FORMAT_LENGTH_MODIFIER_NONE:
            __format_formatPadding(output, str, strLen, fullLen, printArgs);
            break;
        case FORMAT_LENGTH_MODIFIER_L:
            //TODO: Implement wchar_t version here
            break;
        default:
            return -1;
    }

    return fullLen;
}

static Uint64 __format_takeInteger(__FormatLengthModifier modifier, va_list* args) {
    switch (modifier) {
        case FORMAT_LENGTH_MODIFIER_HH:
        case FORMAT_LENGTH_MODIFIER_H:
        case FORMAT_LENGTH_MODIFIER_NONE:
            return (Uint64)va_arg(*args, int);
        case FORMAT_LENGTH_MODIFIER_L:
            return (Uint64)va_arg(*args, long);
        case FORMAT_LENGTH_MODIFIER_LL:
        case FORMAT_LENGTH_MODIFIER_GREAT_L:
            return (Uint64)va_arg(*args, long long);
        case FORMAT_LENGTH_MODIFIER_J:
            return (Uint64)va_arg(*args, Intmax);
        case FORMAT_LENGTH_MODIFIER_Z:
            return (Uint64)va_arg(*args, Size);
        case FORMAT_LENGTH_MODIFIER_T:
            return (Uint64)va_arg(*args, Ptrdiff);
        default:
            return INFINITE;
    }
    return INFINITE;
}

static int __format_handleInteger(Cstring output, Size outputN, Uint64 num, int base, __FormatArgs* printArgs) {
    __format_processDefaultPrecision(printArgs);

    Size numberLen = __format_calculateIntegerLength(num, base, printArgs);
    Size fullLen = algorithms_max32(numberLen, printArgs->width);;
    if (fullLen > outputN) {
        return -1;
    }

    char tmp[numberLen];

    __format_formatInteger(tmp, num, base, printArgs);

    __format_formatPadding(output, tmp, numberLen, fullLen, printArgs);
    
    return fullLen;
}

static void __format_handleCount(__FormatLengthModifier modifier, int ret, va_list* args) {
    switch (modifier) {
        case FORMAT_LENGTH_MODIFIER_HH:
            *((char*)va_arg(*args, char*)) = ret;
            break;
        case FORMAT_LENGTH_MODIFIER_H:
            *((short*)va_arg(*args, short*)) = ret;
            break;
        case FORMAT_LENGTH_MODIFIER_NONE:
            *((int*)va_arg(*args, int*)) = ret;
            break;
        case FORMAT_LENGTH_MODIFIER_L:
            *((long*)va_arg(*args, long*)) = ret;
            break;
        case FORMAT_LENGTH_MODIFIER_LL:
        case FORMAT_LENGTH_MODIFIER_GREAT_L:
            *((long long*)va_arg(*args, long long*)) = ret;
            break;
        case FORMAT_LENGTH_MODIFIER_J:
            *((Uintmax*)va_arg(*args, Uintmax*)) = ret;
            break;
        case FORMAT_LENGTH_MODIFIER_Z:
            *((Size*)va_arg(*args, Size*)) = ret;
            break;
        case FORMAT_LENGTH_MODIFIER_T:
            *((Ptrdiff*)va_arg(*args, Uintptr*)) = ret;
            break;
        default:
    }
}

static void __format_processDefaultPrecision(__FormatArgs* printArgs) {
    if (TEST_FLAGS(printArgs->flags, __FORMAT_FLAGS_NUMBER) && printArgs->precision != -1) {
        SET_FLAG_BACK(printArgs->flags, __FORMAT_FLAGS_LEADING_ZERO);
    }
}

static Size __format_calculateIntegerLength(Uint64 num, int base, __FormatArgs* printArgs) {
    DEBUG_ASSERT_SILENT(base == 8 || base == 10 || base == 16);

    int width = printArgs->width, precision = printArgs->precision;
    Flags8 flags = printArgs->flags;
    
    Size ret = 0;
    if (base == 8 || base == 16) {
        if (TEST_FLAGS(flags, __FORMAT_FLAGS_SPECIFIER)) {
            if (base == 8) {
                ret = 1;
            } else if (base == 16) {
                ret = 2;
            }
        }
    } else if (TEST_FLAGS(flags, __FORMAT_FLAGS_SIGNED)) {
        if ((Int64)num < 0) {
            ret = 1;
            num = -num;
        } else if (TEST_FLAGS(flags, __FORMAT_FLAGS_EXPLICIT_SIGN)) {
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

    if (precision != -1) {
        ret += algorithms_max32(precision, actualLen);
    } else if (width != -1) {
        ret += actualLen;
        ret = TEST_FLAGS(flags, __FORMAT_FLAGS_LEADING_ZERO) ? algorithms_max32(ret, width) : ret;
    } else {
        ret += actualLen;
    }

    return ret;
}

static Size __format_calculateStringLength(ConstCstring str, __FormatArgs* printArgs) {
    Size ret = cstring_strlen(str);
    if (printArgs->precision != -1) {
        ret = algorithms_umin64(ret, printArgs->precision);
    }
    return ret;
}

static ConstCstring _print_digits = "0123456789ABCDEF";
static void __format_formatInteger(Cstring output, Uint64 num, int base, __FormatArgs* printArgs) {
    DEBUG_ASSERT_SILENT(base == 8 || base == 10 || base == 16);

    int width = printArgs->width, precision = printArgs->precision;
    Flags8 flags = printArgs->flags;
    
    int index = 0;

    Uint8 lowercaseBit = TEST_FLAGS(flags, __FORMAT_FLAGS_LOWERCASE) ? 32 : 0;
    if (base == 8 || base == 16) {
        if (TEST_FLAGS(flags, __FORMAT_FLAGS_SPECIFIER)) {
            if (base == 8) {
                output[index++] = '0';
            } else if (base == 16) {
                output[index++] = '0';
                output[index++] = 'X' | lowercaseBit;
            }
        }
    } else if (TEST_FLAGS(flags, __FORMAT_FLAGS_SIGNED)) {              //Sign is not compatible with hex or oct (Forced unsigned)
        if ((Int64)num < 0) {                                           //Negative value
            output[index++] = '-';                                      //Need minus sign
            num = -num;                                                 //Use absolute value
        } else if (TEST_FLAGS(flags, __FORMAT_FLAGS_EXPLICIT_SIGN)) {   //Need explicit sign
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

    int leadingZeroNum = 0;
    if (precision != -1) {
        leadingZeroNum = algorithms_max32(precision - actualLen, 0);
    } else if (width != -1) {
        leadingZeroNum = TEST_FLAGS(flags, __FORMAT_FLAGS_LEADING_ZERO) ? algorithms_max32(width - index - actualLen, 0) : 0;
    }

    for (int i = 0; i < leadingZeroNum; ++i) {
        output[index++] = '0';
    }

    for (int i = actualLen - 1; i >= 0; --i) {
        output[index++] = tmp[i];
    }
}

static void __format_formatPadding(Cstring output, ConstCstring input, Size inputN, Size fullLength, __FormatArgs* printArgs) {
    int lPadding = algorithms_max32(0, fullLength - inputN), rPadding = 0;
    if (TEST_FLAGS(printArgs->flags, __FORMAT_FLAGS_LEFT_JUSTIFY)) {
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