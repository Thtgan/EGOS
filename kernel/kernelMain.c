#include<blowup.h>
#include<devices/hardDisk/hardDisk.h>
#include<devices/keyboard/keyboard.h>
#include<devices/vga/textmode.h>
#include<devices/timer/timer.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/blockDevice/memoryBlockDevice/memoryBlockDevice.h>
#include<fs/fileSystem.h>
#include<interrupt/IDT.h>
#include<kit/oop.h>
#include<memory/buffer.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/virtualMalloc.h>
#include<real/simpleAsmLines.h>
#include<SSE.h>
#include<stdio.h>
#include<string.h>
#include<system/memoryMap.h>
#include<system/systemInfo.h>

SystemInfo* sysInfo;

/**
 * @brief Print the info about memory map, including the num of the detected memory areas, base address, length, type for each areas
 */
static void printMemoryAreas() {
    const MemoryMap* mMap = (const MemoryMap*)sysInfo->memoryMap;

    printf("%d memory areas detected\n", mMap->entryNum);
    printf("|     Base Address     |     Area Length     | Type |\n");
    for (int i = 0; i < mMap->entryNum; ++i) {
        const MemoryMapEntry* e = &mMap->memoryMapEntries[i];
        printf("| %#018llX   | %#018llX  | %#04X |\n", e->base, e->size, e->type);
    }
}

#include<multitask/process.h>

Process p1, p2;

#include<memory/E820.h>
#include<memory/paging/directAccess.h>
#include<memory/physicalMemory/pPageAlloc.h>

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

    initIDT();      //Initialize the interrupt

    //Set interrupt flag
    //Cleared in LINK boot/pm.c#arch_boot_sys_pm_c_cli
    sti();

    printf("EGOS start booting...\n");  //FACE THE SELF, MAKE THE EGOS
    
    printf("MoonLite kernel loading...\n");

    printMemoryAreas();

    printf("Stack: %#018X, StackBase: %#018X\n", readRegister_RSP_64(), readRegister_RBP_64());

    initMemory((void*)readRegister_RBP_64());

    printf("Memory Ready\n");

    printf("Stack: %#018X, StackBase: %#018X\n", readRegister_RSP_64(), readRegister_RBP_64());

    initTimer();

    initKeyboard();

    initBlockDeviceManager();

    initHardDisk();

    BlockDevice* hda = getBlockDeviceByName("hda");
    if (hda == NULL) {
        blowup("Hda not found\n");
    }

    initFileSystem(FILE_SYSTEM_TYPE_PHOSPHERUS);

    FileSystemTypes type = checkFileSystem(hda);
    if (type == FILE_SYSTEM_TYPE_NULL) {
        blowup("File system not installed\n");
    }

    void* buffer = allocateBuffer(BUFFER_SIZE_512);
    FileSystem* fs = openFileSystem(hda, type);

    DirectoryPtr rootDir = fs->pathOperations.getRootDirectory(fs);
    size_t index = fs->pathOperations.searchDirectoryItem(fs, rootDir, "LOGO.bin", false);
    if (index == -1) {
        blowup("File not found\n");
    }
    block_index_t inode = fs->pathOperations.getDirectoryItemInode(fs, rootDir, index);
    FilePtr file = fs->fileOperations.openFile(fs, inode);
    size_t size = fs->fileOperations.getFileSize(fs, file);
    
    fs->fileOperations.seekFile(fs, file, 0);
    fs->fileOperations.readFile(fs, file, buffer, size);
    fs->fileOperations.closeFile(fs, file);

    printf("%s", buffer);

    closeFileSystem(fs);
    releaseBuffer(buffer, BUFFER_SIZE_512);

    Process* mainProcess = initProcess();
    Process* forked = forkFromCurrentProcess("Forked");

    Process* p = PA_TO_DIRECT_ACCESS_VA(getCurrentProcess());

    printf("PID: %u\n", p->pid);
    if (p->pid == 0) {
        printf("This is main process, name: %s\n", p->name);
        switchProcess(mainProcess, forked);
    } else {
        printf("This is child process, name: %s\n", p->name);
        switchProcess(forked, mainProcess);
    }

    p = PA_TO_DIRECT_ACCESS_VA(getCurrentProcess());

    printf("PID: %u\n", p->pid);

    printf("died\n");

    die();
}