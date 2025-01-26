#include<error.h>

#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<debug.h>
#include<print.h>

static ErrorRecord _error_defaultRecord;

ConstCstring error_assertNoneFailStr = "Not expecting any error\n";
ConstCstring error_assertAnyFailStr = "Expecting error not found\n";
ConstCstring error_assertFailStr = "Expecting error %u not found\n";

#define __RESULT_MAXIMUM_ERROR_NUM  256
static int _error_error_num = 0;
static Error _error_errors[__RESULT_MAXIMUM_ERROR_NUM];

void error_registerError(ID errorID, ConstCstring desc) {
    DEBUG_ASSERT_SILENT(_error_error_num < __RESULT_MAXIMUM_ERROR_NUM - 1);
    Error* error = &_error_errors[_error_error_num++];
    error->desc = desc;
}

void error_init() {
    error_registerError(
        ERROR_ID_OK, "OK"
    );

    error_registerError(
        ERROR_ID_UNKNOWN, "Unknown"
    );

    error_registerError(
        ERROR_ID_UNKNOWN, "Illegal Arguments"
    );

    error_registerError(
        ERROR_ID_OUT_OF_MEMORY, "Out Of Memory"
    );

    error_registerError(
        ERROR_ID_DATA_ERROR, "Data Error"
    );

    error_registerError(
        ERROR_ID_NOT_SUPPORTED_OPERATION, "Not Supported Operation"
    );

    error_registerError(
        ERROR_ID_STATE_ERROR, "State Error"
    );

    error_registerError(
        ERROR_ID_ALREADY_EXIST, "Already Exist"
    );

    error_registerError(
        ERROR_ID_NOT_FOUND, "Not Found"
    );

    error_registerError(
        ERROR_ID_OUT_OF_BOUND, "Out Of Bound"
    );

    error_registerError(
        ERROR_ID_OUT_OF_BOUND, "Permission Error"
    );

    error_registerError(
        ERROR_ID_IO_FAILED, "IO Failed"
    );

    error_registerError(
        ERROR_ID_VERIFICATION_FAILED, "Verification Failed"
    );
}

ErrorRecord* error_getCurrentRecord() {
    return &_error_defaultRecord;
}

void error_unhandledRecord(ErrorRecord* record) {
    errorRecord_print(record);
    debug_blowup("Unhandled error\n");
}

void error_readRecord(ErrorRecord* readTo) {
    ErrorRecord* currentRecord = error_getCurrentRecord();
    readTo->errorID = currentRecord->errorID;
    readTo->rip = currentRecord->rip;
    readTo->rsp = currentRecord->rsp;
}

void error_writeRecord(ErrorRecord* writeFrom) {
    ErrorRecord* currentRecord = error_getCurrentRecord();
    currentRecord->errorID = writeFrom->errorID;
    currentRecord->rip = writeFrom->rip;
    currentRecord->rsp = writeFrom->rsp;
}

void errorRecord_initStruct(ErrorRecord* record, ID errorID) {
    record->errorID = errorID;

    if (errorID == 0) {
        record->rip = record->rsp = 0;
    } else {
        Uintptr* stackFrame = (Uintptr*)readRegister_RSP_64();
        record->rip = *(stackFrame);
        record->rsp = (Uintptr)(stackFrame + 1);
    }
}

void errorRecord_print(ErrorRecord* record) {
    ID id = record->errorID;
    DEBUG_ASSERT_SILENT(_error_errors[id].desc != NULL);
    print_printf(TERMINAL_LEVEL_DEBUG, "RIP: %#018lX RSP: %#018lX-%lu(%s)\n", record->rip, record->rsp, id, _error_errors[id].desc);
}
