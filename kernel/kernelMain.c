#include<debug.h>
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

static void printLOGO();

#include<kit/util.h>
#include<system/memoryLayout.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<interrupt/IDT.h>
#include<fs/fat32/fat32.h>
#include<fs/fsutil.h>

extern FileSystem* rootFileSystem;

void kernelMain(SystemInfo* info) {
    sysInfo = (SystemInfo*)info;
    if (sysInfo->magic != SYSTEM_INFO_MAGIC) {
        blowup("Boot magic not match");
    }

    if (initKernel() == RESULT_FAIL) {
        blowup("Initialization failed");
    }

    printLOGO();

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
        printf(TERMINAL_LEVEL_OUTPUT, "USER PROGRAM SHOULD RETURNED\n");

        exitProcess();
        // die();
    }

    kFree(arr1);
    kFree(arr2);
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

static void printLOGO() {
    char buffer[512];
    FileSystemEntryDescriptor desc;
    FileSystemEntry entry;
    if (fileSystemEntryOpen(&entry, &desc, "\\LOGO.txt", FILE_SYSTEM_ENTRY_TYPE_FILE) == RESULT_SUCCESS) {
        fileSeek(&entry, 0, FILE_SEEK_END);
        Size fileSize = fileGetPointer(&entry);
        fileSeek(&entry, 0, FILE_SEEK_BEGIN);
 
        if (fileRead(&entry, buffer, fileSize) == RESULT_SUCCESS) {
            buffer[fileSize] = '\0';
            printf(TERMINAL_LEVEL_OUTPUT, "%s\n", buffer);
        }

        printf(TERMINAL_LEVEL_DEBUG, "%p\n", entry.iNode);
        BlockDevice* device = entry.iNode->superBlock->device;
        fileSystemEntryClose(&entry);
        printf(TERMINAL_LEVEL_DEBUG, "========%s\n", device->name);
        blockDeviceSynchronize(device);
    }
}