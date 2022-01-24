#include<blowup.h>
#include<debug.h>
#include<driver/keyboard/keyboard.h>
#include<driver/vga/textmode.h>
#include<interrupt/IDT.h>
#include<memory/malloc.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<real/simpleAsmLines.h>
#include<stdio.h>
#include<string.h>
#include<system/memoryArea.h>
#include<system/systemInfo.h>

const SystemInfo* systemInfo = (SystemInfo*) SYSTEM_INFO_ADDRESS;

/**
 * @brief Print the info about memory map, including the num of the detected memory areas, base address, length, type for each areas
 */
void printMemoryAreas() {
    const MemoryMap* mMap = systemInfo->memoryMap;

    printf("%d memory areas detected\n", mMap->size);
    printf("| Base Address | Area Length | Type |\n");
    for (int i = 0; i < mMap->size; ++i) {
        const MemoryAreaEntry* e = &mMap->memoryAreas[i];

        printf("| %#010X   | %#010X  | %#04X |\n", 
        (uint32_t)(e->base), 
        (uint32_t)(e->size),
        e->type);
    }
}

void print_storage_info(const int* next, const int* prev, int ints) {
    if (next) {
        printf("%s location: %p. Size: %d ints (%ld bytes).\n",
               (next != prev ? "New" : "Old"), (void*)next, ints, ints * sizeof(int));
    } else {
        printf("Allocation failed.\n");
    }
}

#define __PATTERN_ENTRY(__BACKGROUND_COLOR, __FOREGROUND_COLOR) (uint8_t)VAL_OR(__FOREGROUND_COLOR, VAL_LEFT_SHIFT(__BACKGROUND_COLOR, 4))

__attribute__((section(".kernelMain")))
void kernelMain() {

    initVGATextMode(); //Initialize text mode

    initIDT();      //Initialize the interrupt

    //Set interrupt flag
    //Cleared in LINK arch/boot/sys/pm.c#arch_boot_sys_pm_c_cli
    sti();

    printf("EGOS start booting...\n");  //FACE THE SELF, MAKE THE EGOS
    
    printf("MoonLight kernel loading...\n");

    printMemoryAreas();

    size_t pageNum = initMemory(systemInfo->memoryMap);

    printf("%u pages available\n", pageNum);

    keyboardInit();

    const int pattern[] = {1, 2, 3, 4, 5, 6, 7, 8};
    const int pattern_size = sizeof pattern / sizeof(int);
    int *next = NULL, *prev = NULL;
 
    if ((next = (int*)malloc(pattern_size * sizeof *next))) { // allocates an array
        memcpy(next, pattern, sizeof pattern); // fills the array
        print_storage_info(next, prev, pattern_size);
    } else {
        blowup("Allocation failed");
    }
 
    // Reallocate in cycle using the following values as a new storage size.
    const int realloc_size[] = {10, 12, 512, 32768, 65536, 32768};
 
    for (int i = 0; i != sizeof realloc_size / sizeof(int); ++i) {
        next = (int*)realloc(prev = next, realloc_size[i] * sizeof(int));
        if (next != NULL) {
            print_storage_info(next, prev, realloc_size[i]);
        } else {  // if realloc failed, the original pointer needs to be freed
            free(prev);
            blowup("Allocation failed");
        }
    }
 
    free(next); // finally, frees the storage

    puts("abcd");
    printf("efgh\n");

    die();
}