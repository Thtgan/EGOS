#include<blowup.h>
#include<devices/hardDisk/hardDisk.h>
#include<devices/keyboard/keyboard.h>
#include<devices/vga/textmode.h>
#include<devices/timer/timer.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/blockDevice/memoryBlockDevice/memoryBlockDevice.h>
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

char data[512 * 16];
block_index_t blocks[4096];

#include<fs/phospherus/allocator.h>
#include<fs/phospherus/inode.h>

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

    initAllocator();
    initInode();

    BlockDevice* hda = getBlockDeviceByName("hda");
    if (hda == NULL) {
        blowup("Hda not found\n");
    }

    BlockDevice* memoryDevice = createMemoryBlockDevice(1u << 22); //4MB
    printf("%#llX\n", memoryDevice->additionalData);
    printf("%#llX\n", blocks);
    registerBlockDevice(memoryDevice);

    printBlockDevices();

    deployAllocator(memoryDevice);

    if (!checkBlockDevice(memoryDevice)) {
        blowup("Deploy failed\n");
    }

    Allocator allocator;
    loadAllocator(&allocator, memoryDevice);

    block_index_t inodeBlock = createInode(&allocator, 2048);
    iNodeDesc* inode = openInode(memoryDevice, inodeBlock);


    hda->readBlocks(hda->additionalData, 0, data, 8);
    writeInodeBlocks(inode, data, 8, 8);

    if (!deleteInode(&allocator, inodeBlock)) {
        printf("Delete failed");
    }

    // memset(data, 0, sizeof(data));

    // resizeInode(&allocator, inode->inode, 16);
    // readInodeBlocks(inode->inode, data, 8, 1);
    // printf("%#llX\n", ((uint16_t*)data)[255]);
    // closeInode(memoryDevice, inode);

    // for (int i = 0; i < 4096; ++i) {
    //     blocks[i] = allocateBlock(&allocator);
    // }

    // for (int i = 0; i < 16; ++i) {
    //     hda->readBlocks(hda->additionalData, i, data, 1);
    //     memoryDevice->writeBlocks(memoryDevice->additionalData, blocks[i + 2040], data, 1);
    // }

    // for (int i = 3000; i >= 1000; --i) {
    //     releaseBlock(&allocator, blocks[i]);
    // }

    // printf("%llu\n", getFreeBufferNum(BUFFER_SIZE_512));

    printf("died\n");

    die();
}