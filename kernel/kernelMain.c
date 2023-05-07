#include<debug.h>
#include<devices/block/blockDevice.h>
#include<devices/hardDisk/hardDisk.h>
#include<devices/keyboard/keyboard.h>
#include<devices/terminal/terminal.h>
#include<devices/terminal/terminalSwitch.h>
#include<devices/timer/timer.h>
#include<devices/vga/textmode.h>
#include<devices/virtualDevice.h>
#include<fs/fileSystem.h>
#include<fs/fsutil.h>
#include<interrupt/IDT.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<memory/pageAlloc.h>
#include<memory/paging/paging.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<multitask/semaphore.h>
#include<multitask/spinlock.h>
#include<print.h>
#include<real/simpleAsmLines.h>
#include<string.h>
#include<system/systemInfo.h>
#include<usermode/usermode.h>

SystemInfo* sysInfo;

Semaphore sema1, sema2;
char str[128];

int* arr1, * arr2;

__attribute__((section(".kernelMain"), regparm(2)))
void kernelMain(uint64_t magic, uint64_t sysInfoPtr) {
    sysInfo = (SystemInfo*)sysInfoPtr;

    if (sysInfo->magic != SYSTEM_INFO_MAGIC16) {
        blowup("Magic not match\n");
    }

    initVGATextMode(); //Initialize text mode

    initTerminalSwitch();

    printf(TERMINAL_LEVEL_OUTPUT, "EGOS starts booting...\n");  //FACE THE SELF, MAKE THE EGOS

    initIDT();      //Initialize the interrupt

    initMemory();

    initKeyboard();

    //Set interrupt flag
    //Cleared in LINK boot/pm.c#arch_boot_sys_pm_c_cli
    sti();

    initBlockDeviceManager();

    initHardDisk();

    initSchedule();

    initTimer();

    initFileSystem();
    
    initVirtualDevices();

    initUsermode();

    {
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

    DirectoryEntry entry;
    if (tracePath(&entry, "/dev", INODE_TYPE_DIRECTORY) == RESULT_SUCCESS) {
        printf(TERMINAL_LEVEL_OUTPUT, "Virtual device installed\n");
    }

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
        if (tracePath(&entry, "/dev/tty", INODE_TYPE_DEVICE) == RESULT_FAIL) {
            printf(TERMINAL_LEVEL_OUTPUT, "TTY NOT EXIST\n");
            break;
        }

        printf(TERMINAL_LEVEL_OUTPUT, "ENTRY: %llX %s %u\n", entry.iNodeID, entry.name, entry.type);

        int ttyFile = -1;
        if ((ttyFile = fileOpen("/dev/tty")) == -1) {
            printf(TERMINAL_LEVEL_OUTPUT, "TTY FILE OPEN FAILED, ERROR: %llX\n", getCurrentProcess()->errorCode);
            break;
        }

        File* filePtr = getFileFromSlot(getCurrentProcess(), ttyFile);
        printf(TERMINAL_LEVEL_OUTPUT, "FILE: %p %p %p\n", filePtr, filePtr->iNode, filePtr->operations);
        fileWrite(ttyFile, "TEST TEXT FOR TTY FILE\n", -1);

        fileRead(ttyFile, str, -1);
        printf(TERMINAL_LEVEL_OUTPUT, "TTY READ: %s\n", str);

        fileClose(ttyFile);
    } while (0);

    printf(TERMINAL_LEVEL_OUTPUT, "FINAL %s\n", getCurrentProcess()->name);

    die();
}