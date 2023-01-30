#include<debug.h>
#include<devices/block/blockDevice.h>
#include<devices/hardDisk/hardDisk.h>
#include<devices/keyboard/keyboard.h>
#include<devices/terminal/terminal.h>
#include<devices/terminal/terminalSwitch.h>
#include<devices/timer/timer.h>
#include<devices/vga/textmode.h>
#include<fs/fileSystem.h>
#include<fs/directory.h>
#include<fs/file.h>
#include<fs/inode.h>
#include<interrupt/IDT.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<memory/pageAlloc.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<multitask/semaphore.h>
#include<multitask/spinlock.h>
#include<print.h>
#include<real/simpleAsmLines.h>
#include<SSE.h>
#include<string.h>
#include<system/systemInfo.h>

SystemInfo* sysInfo;

Semaphore sema;

__attribute__((section(".kernelMain"), regparm(2)))
void kernelMain(uint64_t magic, uint64_t sysInfoPtr) {
    sysInfo = (SystemInfo*)sysInfoPtr;

    if (sysInfo->magic != SYSTEM_INFO_MAGIC16) {
        blowup("Magic not match\n");
    }

    if (!checkSSE()) {
        blowup("SSE not supported\n");
    }

    enableSSE();

    initVGATextMode(); //Initialize text mode

    initTerminalSwitch();

    switchTerminalLevel(TERMINAL_LEVEL_OUTPUT);

    printf(TERMINAL_LEVEL_OUTPUT, "EGOS start booting...\n");  //FACE THE SELF, MAKE THE EGOS

    initIDT();      //Initialize the interrupt

    initMemory();

    initKeyboard();

    //Set interrupt flag
    //Cleared in LINK boot/pm.c#arch_boot_sys_pm_c_cli
    sti();

    initBlockDeviceManager();

    initHardDisk();

    BlockDevice* hda = getBlockDeviceByName("hda");
    if (hda == NULL) {
        blowup("Hda not found\n");
    }

    initFileSystem(FILE_SYSTEM_TYPE_PHOSPHERUS);

    initSchedule();

    initTimer();

    if (checkFileSystem(hda) == FILE_SYSTEM_TYPE_PHOSPHERUS) {
        printf(TERMINAL_LEVEL_DEBUG, "File system check passed\n");
    } else {
        blowup("File system check failed\n");
    }

    FileSystem* fs = openFileSystem(hda, FILE_SYSTEM_TYPE_PHOSPHERUS);

    {
        Index64 iNodeIndex = -1;
        iNode* rootDirInode = THIS_ARG_APPEND_CALL(fs, opearations->iNodeGlobalOperations->openInode, fs->rootDirectoryInode);
        Directory* rootDir = fs->opearations->directoryGlobalOperations->openDirectory(rootDirInode);

        Index64 entryIndex = THIS_ARG_APPEND_CALL(rootDir, operations->lookupEntry, "LOGO.bin", INODE_TYPE_FILE);
        DirectoryEntry* entry = THIS_ARG_APPEND_CALL(rootDir, operations->getEntry, entryIndex);
        iNodeIndex = entry->iNodeIndex;
        kFree(entry);

        fs->opearations->directoryGlobalOperations->closeDirectory(rootDir);
        fs->opearations->iNodeGlobalOperations->closeInode(rootDirInode);

        iNode* fileInode = THIS_ARG_APPEND_CALL(fs, opearations->iNodeGlobalOperations->openInode, iNodeIndex);
        File* logoFile = fs->opearations->fileGlobalOperations->openFile(fileInode);

        char* buffer = allocateBuffer(BUFFER_SIZE_512);
        THIS_ARG_APPEND_CALL(logoFile, operations->seek, 0);
        THIS_ARG_APPEND_CALL(logoFile, operations->read, buffer, logoFile->iNode->onDevice.dataSize);
        printf(TERMINAL_LEVEL_OUTPUT, "%s\n", buffer);

        fs->opearations->fileGlobalOperations->closeFile(logoFile);
        fs->opearations->iNodeGlobalOperations->closeInode(fileInode);
    }

    closeFileSystem(fs);

    initSemaphore(&sema, -1);

    if (forkFromCurrentProcess("Forked") != NULL) {
        printf(TERMINAL_LEVEL_OUTPUT, "This is main process, name: %s\n", getCurrentProcess()->name);
    } else {
        printf(TERMINAL_LEVEL_OUTPUT, "This is child process, name: %s\n", getCurrentProcess()->name);
    }

    if (getCurrentProcess()->pid == 0) {
        down(&sema);
        printf(TERMINAL_LEVEL_OUTPUT, "DONE 0\n");
    } else {
        printf(TERMINAL_LEVEL_OUTPUT, "DONE 1\n");
        up(&sema);
        sleep(SECOND, 2);
        up(&sema);
        
        exitProcess();
    }

    printf(TERMINAL_LEVEL_OUTPUT, "FINAL %s\n", getCurrentProcess()->name);

    die();
}