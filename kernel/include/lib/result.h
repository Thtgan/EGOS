#if !defined(__LIB_ERROR_H)
#define __LIB_ERROR_H

typedef struct Error Error;
typedef struct Result Result;

#include<kit/macro.h>
#include<kit/types.h>

#define ERROR_ID_OK                         0
#define ERROR_ID_UNKNOWN                    1
#define ERROR_ID_ILLEGAL_ARGUMENTS          2
#define ERROR_ID_OUT_OF_MEMORY              3
#define ERROR_ID_DATA_ERROR                 4
#define ERROR_ID_NOT_SUPPORTED_OPERATION    5
#define ERROR_ID_STATE_ERROR                6
#define ERROR_ID_ALREADY_EXIST              7
#define ERROR_ID_NOT_FOUND                  8

typedef struct Error {
    ConstCstring desc;
} Error;

void result_registerError(ID errorID, ConstCstring desc);

Result* result_init();

typedef struct Result {
    ID errorID;
    Uintptr rip;
    Uintptr rsp;
} Result;

void result_initStruct(Result* result, ID errorID);

void result_print(Result* result);

Result* result_getCurrentResult();

void result_unhandledError(Result* result);

#define ERROR_THROW_RETURN(__ERROR_ID, ...) do {    \
    Result* _result = result_getCurrentResult();    \
    result_initStruct(_result, __ERROR_ID);         \
    return __VA_OPT__(__VA_ARGS__);                 \
} while(0);

#define ERROR_THROW(__ERROR_ID) do {                \
    Result* _result = result_getCurrentResult();    \
    result_initStruct(_result, __ERROR_ID);         \
    return _result;                                 \
} while(0);

#define ERROR_RETURN_OK()  return NULL

#define ERROR_PASS()       return result_getCurrentResult()

#define ERROR_CHECK_ERROR(__RESULT)    ((__RESULT) != NULL && (__RESULT)->errorID != ERROR_ID_OK)

#define ERROR_RESET()                  result_initStruct(result_getCurrentResult(), ERROR_ID_OK)

#define ERROR_CASE_BUILDER_CALL(__PACKET)  ERROR_CASE_BUILDER __PACKET
#define ERROR_CASE_BUILDER(__ERROR_ID, __CODES)    case __ERROR_ID: {   \
    do __CODES while(0);                                                \
    break;                                                              \
}

#define ERROR_CATCH_RESULT_NAME _result
#define ERROR_CATCH(__RESULT, __DEFAULT_CODES, ...)  do {           \
    Result* ERROR_CATCH_RESULT_NAME = (__RESULT);                   \
    if (!ERROR_CHECK_ERROR(_result)) {                              \
        break;                                                      \
    }                                                               \
    switch (ERROR_CATCH_RESULT_NAME->errorID) {                     \
        FOREACH_MACRO_CALL(ERROR_CASE_BUILDER_CALL, , __VA_ARGS__)  \
        default: __DEFAULT_CODES                                    \
    };                                                              \
} while (0)

#define ERROR_CATCH_DEFAULT_CODES_CRASH             {result_unhandledError(ERROR_CATCH_RESULT_NAME);}
#define ERROR_CATCH_DEFAULT_CODES_PASS              {return ERROR_CATCH_RESULT_NAME;}

#define ERROR_TRY_CATCH(__EXPRESSION, __DEFAULT_CODES, ...) do {            \
    __EXPRESSION;                                                           \
    ERROR_CATCH(result_getCurrentResult(), __DEFAULT_CODES, __VA_ARGS__);   \
} while(0)

#define ERROR_TRY_CATCH_DIRECT(__EXPRESSION, __DEFAULT_CODES, ...) ERROR_CATCH(__EXPRESSION, __DEFAULT_CODES, __VA_ARGS__)

#define ERROR_TRY_BEGIN() do {                                      \
    Result* _tmpCaughtError = NULL;                                 \
    barrier();                                                      \
    Uintptr _returnPoint = debug_getCurrentRIP();                   \
    barrier();                                                      \
    if ((Result*)readMemory64((Uintptr)&_tmpCaughtError) != NULL) { \
        break;                                                      \
    }                                                               \
    barrier()

#define ERROR_TRY_END(__DEFAULT_CODES, ...) ERROR_CATCH(_tmpCaughtError, __DEFAULT_CODES, __VA_ARGS__); } while(0)

#define ERROR_TRY_CALL(__EXPRESSION)    do {                    \
    __EXPRESSION;                                               \
    Result* _tmpRes = result_getCurrentResult();                \
    if (_tmpRes->errorID == ERROR_ID_OK) {                      \
        break;                                                  \
    }                                                           \
    writeMemory64((Uintptr)&_tmpCaughtError, (Uint64)_tmpRes);  \
    asm volatile("jmp *%0" :: "r"(_returnPoint));               \
} while(0)

#define ERROR_TRY_CALL_DIRECT(__EXPRESSION)    do {             \
    Result* _tmpRes = __EXPRESSION;                             \
    if (_tmpRes == NULL) {                                      \
        break;                                                  \
    }                                                           \
    writeMemory64((Uintptr)&_tmpCaughtError, (Uint64)_tmpRes);  \
    asm volatile("jmp *%0" :: "r"(_returnPoint));               \
} while(0)

#endif // __LIB_ERROR_H
