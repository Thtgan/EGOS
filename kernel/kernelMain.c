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
#include<time/time.h>
#include<time/timer.h>
#include<usermode/usermode.h>

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

extern FS* rootFS;

void kernelMain(SystemInfo* info) {
    sysInfo = (SystemInfo*)info;
    if (sysInfo->magic != SYSTEM_INFO_MAGIC) {
        debug_blowup("Boot magic not match");
    }

    if (initKernel() == RESULT_FAIL) {
        debug_blowup("Initialization failed");
    }

    printLOGO();

    initSemaphore(&sema1, 0);
    initSemaphore(&sema2, -1);

    arr1 = kMallocSpecific(1 * sizeof(int), PHYSICAL_PAGE_ATTRIBUTE_COW, 16), arr2 = kMallocSpecific(1 * sizeof(int), PHYSICAL_PAGE_ATTRIBUTE_PUBLIC, 16); //TODO: COW not working
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

        int ret = execute("\\bin\\test");
        printf(TERMINAL_LEVEL_OUTPUT, "USER PROGRAM RETURNED %d\n", ret);

        fs_close(rootFS); //TODO: Move to better place

        up(&sema2);
        exitProcess();
    }

    kFree(arr1);
    kFree(arr2);
    // do {
    //     File* ttyFile = NULL;
    //     if ((ttyFile = fileOpen("/dev/tty", FILE_FLAG_READ_WRITE)) == NULL) {
    //         printf(TERMINAL_LEVEL_OUTPUT, "TTY FILE OPEN FAILED, ERROR: %llX\n", schedulerGetCurrentProcess()->errorCode);
    //         break;
    //     }

    //     fsutil_fileWrite(ttyFile, "TEST TEXT FOR TTY FILE\n", -1);

    //     fsutil_fileRead(ttyFile, str, -1);
    //     printf(TERMINAL_LEVEL_OUTPUT, "TTY READ: %s\n", str);

    //     fileClose(ttyFile);
    // } while (0);

    printf(TERMINAL_LEVEL_OUTPUT, "FINAL %s\n", schedulerGetCurrentProcess()->name);


    Timer timer1, timer2;
    initTimer(&timer1, 500, TIME_UNIT_MILLISECOND);
    initTimer(&timer2, 500, TIME_UNIT_MILLISECOND);
    SET_FLAG_BACK(timer2.flags, TIMER_FLAGS_SYNCHRONIZE | TIMER_FLAGS_REPEAT);

    timer1.handler = LAMBDA(void, (Timer* timer) {
        printf(TERMINAL_LEVEL_OUTPUT, "HANDLER CALL FROM TIMER1\n");
    });

    timer2.data = 5;
    timer2.handler = LAMBDA(void, (Timer* timer) {
        printf(TERMINAL_LEVEL_OUTPUT, "HANDLER CALL FROM TIMER2\n");
        if (--timer->data == 0) {
            CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_REPEAT);
        }
    });


    timerStart(&timer1);
    timerStart(&timer2);

    printf(TERMINAL_LEVEL_OUTPUT, "DEAD\n");

    die();
}

static void printLOGO() {
    char buffer[1024];
    FSentryDesc desc;
    FSentry entry;
    if (fsutil_openFSentry(&entry, &desc, "\\LOGO.txt", FS_ENTRY_TYPE_FILE) == RESULT_SUCCESS) {
        fsutil_fileSeek(&entry, 0, FILE_SEEK_END);
        Size fileSize = fsutil_fileGetPointer(&entry);
        fsutil_fileSeek(&entry, 0, FILE_SEEK_BEGIN);

        if (fsutil_fileRead(&entry, buffer, fileSize) == RESULT_SUCCESS) {
            buffer[fileSize] = '\0';
            printf(TERMINAL_LEVEL_OUTPUT, "%s\n", buffer);
        }

        // if (fileSize < 0x200) {
        //     fsutil_fileWrite(&entry, "Per Aspera Ad Astra\nPer Aspera Ad Astra\nPer Aspera Ad Astra\n", 60);
        // }
        // printf(TERMINAL_LEVEL_OUTPUT, "%lu\n", desc.createTime);
        // printf(TERMINAL_LEVEL_OUTPUT, "%lu\n", desc.lastAccessTime);
        // printf(TERMINAL_LEVEL_OUTPUT, "%lu\n", desc.lastModifyTime);

        BlockDevice* device = entry.iNode->superBlock->device;
        fsutil_closeFSentry(&entry);
        blockDeviceSynchronize(device);
    }
}