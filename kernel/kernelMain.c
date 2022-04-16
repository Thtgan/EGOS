#include<blowup.h>
#include<devices/hardDisk/hardDisk.h>
#include<devices/keyboard/keyboard.h>
#include<devices/vga/textmode.h>
#include<devices/timer/timer.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/blockDevice/memoryBlockDevice/memoryBlockDevice.h>
#include<fs/fileSystem.h>
#include<interrupt/IDT.h>
#include<memory/buffer.h>
#include<memory/malloc.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<real/simpleAsmLines.h>
#include<SSE.h>
#include<stdio.h>
#include<string.h>
#include<system/memoryMap.h>
#include<system/systemInfo.h>

const SystemInfo* systemInfo;

/**
 * @brief Print the info about memory map, including the num of the detected memory areas, base address, length, type for each areas
 */
void printMemoryAreas() {
    const MemoryMap* mMap = (const MemoryMap*)systemInfo->memoryMap;

    printf("%d memory areas detected\n", mMap->size);
    printf("|     Base Address     |     Area Length     | Type |\n");
    for (int i = 0; i < mMap->size; ++i) {
        const MemoryMapEntry* e = &mMap->memoryMapEntries[i];
        printf("| %#018llX   | %#018llX  | %#04X |\n", e->base, e->size, e->type);
    }
}

__attribute__((section(".kernelMain"), regparm(2)))
void kernelMain(uint64_t magic, uint64_t sysInfo) {
    systemInfo = (SystemInfo*)sysInfo;

    if (systemInfo->magic != SYSTEM_INFO_MAGIC16) {
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

    size_t pageNum = initMemory(systemInfo);

    printf("%u pages available\n", pageNum);

    initBuffer();

    initTimer();

    initKeyboard();

    initBlockDeviceManager();

    initHardDisk();

    // phospherus_initAllocator();
    // phospherus_initInode();

    BlockDevice* hda = getBlockDeviceByName("hda");
    if (hda == NULL) {
        blowup("Hda not found\n");
    }

    BlockDevice* memoryDevice = createMemoryBlockDevice(1u << 22); //4MB
    printf("%#llX\n", memoryDevice->additionalData);
    registerBlockDevice(memoryDevice);

    printBlockDevices();

    initFileSystem(FILE_SYSTEM_TYPE_PHOSPHERUS);
    if (deployFileSystem(memoryDevice, FILE_SYSTEM_TYPE_PHOSPHERUS)) {
        printf("File system deployed\n");
    } else {
        blowup("File system deploy failed\n");
    }

    void* buffer = allocateBuffer(BUFFER_SIZE_64);

    FileSystemTypes type = checkFileSystem(memoryDevice);
    if (type == FILE_SYSTEM_TYPE_NULL) {
        printf("File system not installed\n");
    }

    FileSystem* fs = openFileSystem(memoryDevice, type);
    printf("%s\n", fs->name);

    DirectoryPtr rootDir = fs->pathOperations.getRootDirectory(fs);
    block_index_t newFileInode = fs->fileOperations.createFile(fs);
    fs->pathOperations.insertDirectoryItem(fs, rootDir, newFileInode, "test.txt", false);
    fs->pathOperations.readDirectoryItemName(fs, rootDir, 0, buffer, 63);
    printf("%s\n", buffer);

    FilePtr file = fs->fileOperations.openFile(fs, fs->pathOperations.getDirectoryItemInode(fs, rootDir, 0));
    fs->fileOperations.seekFile(fs, file, 0);
    fs->fileOperations.writeFile(fs, file, buffer, strlen(buffer) + 1);
    fs->fileOperations.closeFile(fs, file);

    memset(buffer, 0, sizeof(buffer));
    file = fs->fileOperations.openFile(fs, fs->pathOperations.getDirectoryItemInode(fs, rootDir, 0));
    size_t size = fs->fileOperations.getFileSize(fs, file);
    fs->fileOperations.readFile(fs, file, buffer, size);
    printf("%s %u\n", buffer, size);
    fs->fileOperations.closeFile(fs, file);

    block_index_t newDirectoryInode = fs->pathOperations.createDirectory(fs);
    fs->pathOperations.insertDirectoryItem(fs, rootDir, newDirectoryInode, "testDir", true);
    DirectoryPtr subDirectory = fs->pathOperations.openDirectory(fs, fs->pathOperations.getDirectoryItemInode(fs, rootDir, 1));
    printf("%u %u\n", fs->pathOperations.getDirectoryItemNum(fs, rootDir), fs->pathOperations.getDirectoryItemNum(fs, subDirectory));

    for (int i = 0; i < 2; ++i) {
        fs->pathOperations.readDirectoryItemName(fs, rootDir, i, buffer, 63);
        printf("%s\n", buffer);
    }

    block_index_t removed = fs->pathOperations.removeDirectoryItem(fs, rootDir, 0);
    fs->fileOperations.deleteFile(fs, removed);
    printf("%u\n", fs->pathOperations.getDirectoryItemNum(fs, rootDir));
    fs->pathOperations.readDirectoryItemName(fs, rootDir, 0, buffer, 63);
    printf("%s\n", buffer);

    closeFileSystem(fs);
    releaseBuffer(buffer, BUFFER_SIZE_64);
    
    printf("died\n");

    die();
}