#include<fs/fsutil.h>
#include<init.h>
#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<multitask/process.h>
#include<multitask/semaphore.h>
#include<print.h>
#include<real/simpleAsmLines.h>
#include<string.h>
#include<system/systemInfo.h>
#include<usermode/usermode.h>

SystemInfo* sysInfo;

Semaphore sema1, sema2;
char str[128];

int* arr1, * arr2;

static void printLOGO();

__attribute__((section(".kernelMain"), regparm(2)))
void kernelMain(uint64_t magic, uint64_t sysInfoPtr) {
    sysInfo = (SystemInfo*)sysInfoPtr;
    if (sysInfo->magic != SYSTEM_INFO_MAGIC16) {
        die();
    }

    if (initKernel() == RESULT_FAIL) {
        die();
    }

    printLOGO();

    initSemaphore(&sema1, 0);
    initSemaphore(&sema2, -1);

    arr1 = kMalloc(1 * sizeof(int), MEMORY_TYPE_NORMAL), arr2 = kMalloc(1 * sizeof(int), MEMORY_TYPE_SHARE);
    arr1[0] = 1, arr2[0] = 114514;
    if (forkFromCurrentProcess("Forked") != NULL) {
        printf(TERMINAL_LEVEL_OUTPUT, "This is main process, name: %s\n", getCurrentProcess()->name);
    } else {
        printf(TERMINAL_LEVEL_OUTPUT, "This is child process, name: %s\n", getCurrentProcess()->name);
    }

    if (getCurrentProcess()->pid == 0) {
        arr1[0] = 3;
        up(&sema1);
        down(&sema2);
        printf(TERMINAL_LEVEL_OUTPUT, "DONE 0, arr1: %d, arr2: %d\n", arr1[0], arr2[0]);
    } else {
        down(&sema1);
        arr1[0] = 2, arr2[0] = 1919810;
        printf(TERMINAL_LEVEL_OUTPUT, "DONE 1, arr1: %d, arr2: %d\n", arr1[0], arr2[0]);
        up(&sema2);
        while (true) {
            int len = terminalGetline(getLevelTerminal(TERMINAL_LEVEL_OUTPUT), str);

            if (strcmp(str, "PASS") == 0 || len == 0) {
                break;
            }
            printf(TERMINAL_LEVEL_OUTPUT, "%s-%d\n", str, len);
        }
        up(&sema2);

        int ret = execute("/bin/test");
        printf(TERMINAL_LEVEL_OUTPUT, "USER PROGRAM RETURNED %d\n", ret);

        exitProcess();
    }

    kFree(arr1);
    kFree(arr2);

    do {
        int ttyFile = -1;
        if ((ttyFile = fileOpen("/dev/tty")) == -1) {
            printf(TERMINAL_LEVEL_OUTPUT, "TTY FILE OPEN FAILED, ERROR: %llX\n", getCurrentProcess()->errorCode);
            break;
        }

        fileWrite(ttyFile, "TEST TEXT FOR TTY FILE\n", -1);

        fileRead(ttyFile, str, -1);
        printf(TERMINAL_LEVEL_OUTPUT, "TTY READ: %s\n", str);

        fileClose(ttyFile);
    } while (0);

    printf(TERMINAL_LEVEL_OUTPUT, "FINAL %s\n", getCurrentProcess()->name);

    die();
}

static void printLOGO() {
    char* buffer = allocateBuffer(BUFFER_SIZE_512);
    int file = fileOpen("/LOGO.txt");
    fileSeek(file, 0, FSUTIL_SEEK_END);
    size_t fileSize = fileGetPointer(file);
    fileSeek(file, 0, FSUTIL_SEEK_BEGIN);
    size_t read = fileRead(file, buffer, fileSize);
    printf(TERMINAL_LEVEL_OUTPUT, "%u bytes read:\n%s\n", fileSize, buffer);
    fileClose(file);
    releaseBuffer(buffer, BUFFER_SIZE_512);
}