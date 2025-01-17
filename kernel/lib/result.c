#include<result.h>

#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<debug.h>
#include<print.h>

static Result _result_defaultResult;

#define __RESULT_MAXIMUM_ERROR_NUM  256
static int _result_error_num = 0;
static Error _result_errors[__RESULT_MAXIMUM_ERROR_NUM];

void result_registerError(ID errorID, ConstCstring desc) {
    DEBUG_ASSERT_SILENT(_result_error_num < __RESULT_MAXIMUM_ERROR_NUM - 1);
    Error* error = &_result_errors[_result_error_num++];
    error->desc = desc;
}

Result* result_getCurrentResult() {
    result_registerError(
        ERROR_ID_OK, "OK"
    );

    result_registerError(
        ERROR_ID_UNKNOWN, "Unknown"
    );

    result_registerError(
        ERROR_ID_UNKNOWN, "Illegal Arguments"
    );

    result_registerError(
        ERROR_ID_OUT_OF_MEMORY, "Out Of Memory"
    );

    result_registerError(
        ERROR_ID_DATA_ERROR, "Data Error"
    );

    result_registerError(
        ERROR_ID_NOT_SUPPORTED_OPERATION, "Not Supported Operation"
    );
    
    return &_result_defaultResult;
}

void result_initStruct(Result* result, ID errorID) {
    result->errorID = errorID;

    if (errorID == 0) {
        result->rip = result->rsp = 0;
    } else {
        Uintptr* stackFrame = (Uintptr*)readRegister_RBP_64();
        result->rip = *(stackFrame + 1);
        result->rsp = (Uintptr)(stackFrame + 2);
    }
}

void result_print(Result* result) {
    ID id = result->errorID;
    print_printf(TERMINAL_LEVEL_DEBUG, "RIP: %#018lX RSP: %#018lX-%lu(%s)\n", result->rip, result->rsp, id, _result_errors[id].desc);
}

void result_unhandledError(Result* result) {
    result_print(result);
    debug_blowup("Unhandled error\n");
}