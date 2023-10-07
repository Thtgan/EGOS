// #include<fs/fsutil.h>
#include<init.h>
// #include<kit/types.h>
// #include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<multitask/schedule.h>
// #include<multitask/semaphore.h>
#include<print.h>
#include<real/simpleAsmLines.h>
#include<string.h>
#include<system/systemInfo.h>
// #include<usermode/usermode.h>

// #include<memory/physicalPages.h>

SystemInfo* sysInfo;

Semaphore sema1, sema2;
char str[128];

int* arr1, * arr2;

// static void printLOGO();

#include<kit/util.h>
#include<system/memoryLayout.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<interrupt/IDT.h>

void kernelMain(SystemInfo* info) {
    sysInfo = (SystemInfo*)info;
    if (sysInfo->magic != SYSTEM_INFO_MAGIC) {
        blowup("Boot magic not match");
    }

    if (initKernel() == RESULT_FAIL) {
        blowup("Initialization failed");
    }

    printf(TERMINAL_LEVEL_OUTPUT, "TEST\n");\

    printf(TERMINAL_LEVEL_OUTPUT, "%X %X\n", mm->directPageTableBegin, mm->directPageTableEnd);
    printf(TERMINAL_LEVEL_OUTPUT, "%X %X\n", mm->physicalPageStructBegin, mm->physicalPageStructEnd);
    printf(TERMINAL_LEVEL_OUTPUT, "%X %X\n", mm->freePageBegin, mm->freePageEnd);

//     printLOGO();

    initSemaphore(&sema1, 0);
    initSemaphore(&sema2, -1);

    arr1 = kMallocSpecific(1 * sizeof(int), MEMORY_TYPE_NORMAL, 16), arr2 = kMallocSpecific(1 * sizeof(int), MEMORY_TYPE_PUBLIC, 16);
    arr1[0] = 1, arr2[0] = 114514;
    if (fork("Forked") != NULL) {
        printf(TERMINAL_LEVEL_OUTPUT, "This is main process, name: %s\n", schedulerGetCurrentProcess()->name);
    } else {
        printf(TERMINAL_LEVEL_OUTPUT, "This is child process, name: %s\n", schedulerGetCurrentProcess()->name);
    }

    if (strcmp(schedulerGetCurrentProcess()->name, "Init") == 0) {
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
            printf(TERMINAL_LEVEL_OUTPUT, "Waiting for input: ");
            int len = terminalGetline(getLevelTerminal(TERMINAL_LEVEL_OUTPUT), str);

            if (strcmp(str, "PASS") == 0 || len == 0) {
                break;
            }
            printf(TERMINAL_LEVEL_OUTPUT, "%s-%d\n", str, len);
        }
        up(&sema2);

        // int ret = execute("/bin/test");
        // printf(TERMINAL_LEVEL_OUTPUT, "USER PROGRAM RETURNED %d\n", ret);
        printf(TERMINAL_LEVEL_OUTPUT, "USER PROGRAM SHOULD RETURNED");

        exitProcess();
        // die();
    }

    kFree(arr1);
    kFree(arr2);

    BlockDevice* device = getBlockDeviceByName("HDA");
    char buffer[512];
    if (blockDeviceReadBlocks(device, 0, buffer, 1) == RESULT_SUCCESS) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 16; ++j) {
                printf(TERMINAL_LEVEL_OUTPUT, "%02X ", (Uint8)buffer[16 * i + j]);
            }
            putchar(TERMINAL_LEVEL_OUTPUT, '\n');
        }

        printf(TERMINAL_LEVEL_OUTPUT, "================\n");

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 16; ++j) {
                printf(TERMINAL_LEVEL_OUTPUT, "%02X ", (Uint8)buffer[448 + 16 * i + j]);
            }
            putchar(TERMINAL_LEVEL_OUTPUT, '\n');
        }
    }
    

//     do {
//         File* ttyFile = NULL;
//         if ((ttyFile = fileOpen("/dev/tty", FILE_FLAG_READ_WRITE)) == NULL) {
//             printf(TERMINAL_LEVEL_OUTPUT, "TTY FILE OPEN FAILED, ERROR: %llX\n", schedulerGetCurrentProcess()->errorCode);
//             break;
//         }

//         fileWrite(ttyFile, "TEST TEXT FOR TTY FILE\n", -1);

//         fileRead(ttyFile, str, -1);
//         printf(TERMINAL_LEVEL_OUTPUT, "TTY READ: %s\n", str);

//         fileClose(ttyFile);
//     } while (0);

//     printf(TERMINAL_LEVEL_OUTPUT, "FINAL %s\n", schedulerGetCurrentProcess()->name);

    die();
}

// static void printLOGO() {
//     char* buffer = allocateBuffer(BUFFER_SIZE_512);
//     File* file = fileOpen("/LOGO.txt", FILE_FLAG_READ_ONLY);
//     fileSeek(file, 0, FSUTIL_SEEK_END);
//     Size fileSize = fileGetPointer(file);
//     fileSeek(file, 0, FSUTIL_SEEK_BEGIN);
//     Size read = fileRead(file, buffer, fileSize);
//     printf(TERMINAL_LEVEL_OUTPUT, "%u bytes read:\n%s\n", fileSize, buffer);
//     fileClose(file);
//     releaseBuffer(buffer, BUFFER_SIZE_512);
// }