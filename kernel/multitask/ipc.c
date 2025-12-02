#include<multitask/ipc.h>

#include<multitask/pipe.h>
#include<error.h>

void ipc_pipe(int* writeRet, int* readRet) {
    File* writeFile, * readFile;
    pipe_create(&writeFile, &readFile);
    ERROR_GOTO_IF_ERROR(0);

    Process* process = schedule_getCurrentProcess();
    *writeRet = process_addFSentry(process, writeFile);
    *readRet = process_addFSentry(process, readFile);

    return;

    ERROR_FINAL_BEGIN(0);
    *writeRet = *readRet = 0;
    return;
}