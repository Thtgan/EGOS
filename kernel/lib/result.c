#include<result.h>

#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<debug.h>
#include<print.h>

static Result _result_defaultResult;

#define __RESULT_MAXIMUM_ERROR_NUM  256
static Error _result_errors[__RESULT_MAXIMUM_ERROR_NUM];

Result* result_getCurrentResult() {
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