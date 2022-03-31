#include<blowup.h>
#include<devices/hardDisk/hardDisk.h>
#include<devices/keyboard/keyboard.h>
#include<devices/vga/textmode.h>
#include<devices/timer/timer.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/blockDevice/memoryBlockDevice/memoryBlockDevice.h>
#include<interrupt/IDT.h>
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

char data[512];
BlockDevice memoryDevice;

__attribute__((section(".kernelMain"), regparm(2)))
void kernelMain(uint64_t magic, uint64_t sysInfo) {
    systemInfo = (SystemInfo*)sysInfo;

    if (systemInfo->magic != SYSTEM_INFO_MAGIC) {
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

    initTimer();

    initKeyboard();

    initBlockDeviceManager();

    initHardDisk();

    initMemoryBlockDevice(&memoryDevice, 1u << 20);
    registerBlockDevice(&memoryDevice);

    printBlockDevices();

    BlockDevice* device = getBlockDeviceByName("hda");
    printf("%p\n", device->additionalData);
    device->readBlock(device->additionalData, 0, data);

    printf("%016llX\n", *((uint64_t*)data));

    // BlockDevice* device = getBlockDeviceByName("memory0");
    // printf("%p\n", device->additionalData);
    // device->readBlock(device->additionalData, 0, data);

    // printf("%016llX\n", *((uint64_t*)data));

    // *((uint64_t*)data) = 1234567890;
    
    // device->writeBlock(device->additionalData, 0, data);

    // memset(data, 0, 512);

    // device->readBlock(device->additionalData, 0, data);

    // printf("%016llX\n", *((uint64_t*)data));

    die();
}