#if !defined(__LIB_ERROR_H)
#define __LIB_ERROR_H

typedef struct Error Error;
typedef struct ErrorRecord ErrorRecord;

#include<debug.h>
#include<kit/macro.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>

#define ERROR_ID_OK                         0
#define ERROR_ID_UNKNOWN                    1
#define ERROR_ID_ILLEGAL_ARGUMENTS          2
#define ERROR_ID_OUT_OF_MEMORY              3
#define ERROR_ID_DATA_ERROR                 4
#define ERROR_ID_NOT_SUPPORTED_OPERATION    5
#define ERROR_ID_STATE_ERROR                6
#define ERROR_ID_ALREADY_EXIST              7
#define ERROR_ID_NOT_FOUND                  8
#define ERROR_ID_OUT_OF_BOUND               9
#define ERROR_ID_PERMISSION_ERROR           10
#define ERROR_ID_IO_FAILED                  11
#define ERROR_ID_VERIFICATION_FAILED        12

typedef struct Error {
    ConstCstring desc;
} Error;

void error_registerError(ID errorID, ConstCstring desc);

void error_init();

ErrorRecord* error_getCurrentRecord();

void error_unhandledRecord(ErrorRecord* record);

typedef struct ErrorRecord {
    ID errorID;
    Uintptr rip;
    Uintptr rsp;
} ErrorRecord;

void errorRecord_initStruct(ErrorRecord* record, ID errorID);

void errorRecord_print(ErrorRecord* record);

static inline void errorRecord_setError(ID errorID) {
    errorRecord_initStruct(error_getCurrentRecord(), errorID);
} 

#define ERROR_CATCH_RESULT_NAME _errorRecord

#define ERROR_FINAL_BEGIN_LABEL(__ID)   MACRO_CONCENTRATE2(errorFinal_, __ID)

#define ERROR_FINAL_BEGIN(__GOTO_ID)    ERROR_FINAL_BEGIN_LABEL(__GOTO_ID): barrier()

#define ERROR_THROW(__ERROR_ID, __GOTO_ID) do { \
    errorRecord_setError(__ERROR_ID);           \
    goto ERROR_FINAL_BEGIN_LABEL(__GOTO_ID);    \
} while(0)

#define ERROR_THROW_NO_GOTO(__ERROR_ID) errorRecord_setError(__ERROR_ID)

#define ERROR_CLEAR() errorRecord_setError(ERROR_ID_OK)

#define ERROR_GOTO(__GOTO_ID) goto ERROR_FINAL_BEGIN_LABEL(__GOTO_ID)

#define ERROR_GOTO_IF_ERROR(__GOTO_ID)  do {                \
    if (error_getCurrentRecord()->errorID != ERROR_ID_OK) { \
        goto ERROR_FINAL_BEGIN_LABEL(__GOTO_ID);            \
    }                                                       \
} while(0)

extern ConstCstring error_assertNoneFailStr;
extern ConstCstring error_assertAnyFailStr;
extern ConstCstring error_assertFailStr;

#define ERROR_ASSERT_NONE()         do { if (error_getCurrentRecord()->errorID != ERROR_ID_OK) { debug_dump_stack(NULL, INFINITE); debug_blowup(error_assertNoneFailStr); } } while(0)

#define ERROR_ASSERT_ANY()          do { if (error_getCurrentRecord()->errorID == ERROR_ID_OK) { debug_dump_stack(NULL, INFINITE); debug_blowup(error_assertAnyFailStr); } } while(0)

#define ERROR_ASSERT(__ERROR_ID)    do { if (error_getCurrentRecord()->errorID != __ERROR_ID) { debug_dump_stack(NULL, INFINITE); debug_blowup(error_assertFailStr, __ERROR_ID); } } while(0)

#define ERROR_CHECKPOINT_DEFAULT_CODES  {error_unhandledRecord(ERROR_CATCH_RESULT_NAME);}

#define ERROR_CASE_BUILDER_CALL(__PACKET)  ERROR_CASE_BUILDER __PACKET
#define ERROR_CASE_BUILDER(__ERROR_ID, __CODES)    case __ERROR_ID: {   \
    do __CODES while(0);                                                \
    break;                                                              \
}

#define ERROR_CHECKPOINT(__DEFAULT_CODES, ...)  do {                                        \
    ErrorRecord* ERROR_CATCH_RESULT_NAME = error_getCurrentRecord();                        \
    if ((ERROR_CATCH_RESULT_NAME)->errorID == ERROR_ID_OK) {                                \
        break;                                                                              \
    }                                                                                       \
    switch ((ERROR_CATCH_RESULT_NAME)->errorID) {                                           \
        MACRO_FOREACH_CALL(ERROR_CASE_BUILDER_CALL, , __VA_ARGS__)                          \
        default:                                                                            \
        MACRO_IF_EMPTY(__DEFAULT_CODES, __DEFAULT_CODES, ERROR_CHECKPOINT_DEFAULT_CODES);   \
    };                                                                                      \
} while (0)

#endif // __LIB_ERROR_H
