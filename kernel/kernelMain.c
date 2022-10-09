#include<blowup.h>
#include<devices/block/blockDevice.h>
#include<devices/hardDisk/hardDisk.h>
#include<devices/keyboard/keyboard.h>
#include<devices/vga/textmode.h>
#include<devices/timer/timer.h>
#include<interrupt/IDT.h>
#include<kit/oop.h>
#include<memory/buffer.h>
#include<memory/memory.h>
#include<memory/paging/directAccess.h>
#include<memory/paging/paging.h>
#include<memory/virtualMalloc.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<real/simpleAsmLines.h>
#include<SSE.h>
#include<stdio.h>
#include<string.h>
#include<structs/hashTable.h>
#include<system/memoryMap.h>
#include<system/systemInfo.h>

#include<devices/block/memoryBlockDevice.h>
#include<fs/fileSystem.h>
#include<fs/file.h>
#include<fs/directory.h>
#include<fs/inode.h>

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

char buffer[256];

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

    BlockDevice* mDevice = createMemoryBlockDevice(2 * MB);
    registerBlockDevice(mDevice);
    printf("Memory block device begins at: %p\n", mDevice->additionalData);

    initFileSystem(FILE_SYSTEM_TYPE_PHOSPHERUS);
    if (checkFileSystem(hda) == FILE_SYSTEM_TYPE_PHOSPHERUS) {
        printf("File system check passed\n");
    } else {
        printf("File system check failed\n");
    }

    FileSystem* fs = openFileSystem(hda, FILE_SYSTEM_TYPE_PHOSPHERUS);

    {
        Index64 iNodeIndex = -1;
        iNode* rootDirInode = fs->opearations->iNodeGlobalOperations->openInode(fs->device, fs->rootDirectoryInode);
        Directory* rootDir = fs->opearations->directoryGlobalOperations->openDirectory(rootDirInode);

        Index64 entryIndex = rootDir->operations->lookupEntry(rootDir, "LOGO.bin", INODE_TYPE_FILE);
        DirectoryEntry* entry = rootDir->operations->getEntry(rootDir, entryIndex);
        iNodeIndex = entry->iNodeIndex;
        vFree(entry);

        fs->opearations->directoryGlobalOperations->closeDirectory(rootDir);
        fs->opearations->iNodeGlobalOperations->closeInode(rootDirInode);

        iNode* fileInode = fs->opearations->iNodeGlobalOperations->openInode(fs->device, iNodeIndex);
        File* logoFile = fs->opearations->fileGlobalOperations->openFile(fileInode);

        char* buffer = allocateBuffer(BUFFER_SIZE_512);
        logoFile->operations->seek(logoFile, 0);
        logoFile->operations->read(logoFile, buffer, logoFile->iNode->onDevice.dataSize);
        printf("%s\n", buffer);

        fs->opearations->fileGlobalOperations->closeFile(logoFile);
        fs->opearations->iNodeGlobalOperations->closeInode(fileInode);
    }

    closeFileSystem(fs);

    initSchedule();
    Process* mainProcess = initProcess();
    Process* forked = forkFromCurrentProcess("Forked");
    Process* p = PA_TO_DIRECT_ACCESS_VA(getCurrentProcess());

    printf("PID: %u\n", p->pid);
    if (p->pid == 0) {
        printf("This is main process, name: %s\n", p->name);
    } else {
        printf("This is child process, name: %s\n", p->name);
    }

    p = PA_TO_DIRECT_ACCESS_VA(getCurrentProcess());

    printf("PID: %u\n", p->pid);

    blowup("DEAD\n");
}