#include<driver/disk.h>
#include<elf/elf.h>
#include<fs/fat32/fat32.h>
#include<fs/fileSystem.h>
#include<fs/volume.h>
#include<kit/types.h>
#include<kit/util.h>
#include<lib/blowup.h>
#include<lib/intn.h>
#include<lib/print.h>
#include<lib/singlyLinkedList.h>
#include<mm/mm.h>
#include<real/simpleAsmLines.h>
#include<system/A20.h>
#include<system/E820.h>
#include<system/paging.h>

extern void enterKernel(void* entry, MemoryMap* mMap);

MemoryMap mMap;

typedef void (*KernelEntryType)();

__attribute__((noreturn))
void bootEntry32(int drive) {
    print("Entering bootloader...\n");

    if (initA20() == RESULT_SUCCESS) {
        print("A20 enabled\n");
    } else {
        blowup("Error occured when enabling A20.\n");
    }

    if (initE820(&mMap) == RESULT_SUCCESS) {
        printf("%d memory areas detected\n", mMap.entryNum);
        print("| Base Address | Area Length | Type |\n");
        for (int i = 0; i < mMap.entryNum; ++i) {
            const MemoryMapEntry* e = mMap.memoryMapEntries + i;
            printf("|  %#010X  | %#010X  | %#04X |\n", (Uint32)e->base, (Uint32)e->length, e->type);
        }
    } else {
        blowup("Error occured when initializing E820.\n");
    }

    if (initMemoryManager(&mMap) == RESULT_SUCCESS) {
        print("Memory manager initialized\n");
    } else {
        blowup("Error occured when initializing memory manager.\n");
    }

    initPaging();

    Volume* driveVolume = scanVolume(drive);
    if (driveVolume == NULL) {
        blowup("Error occured when scanning volume.\n");
    } else {
        printf("%d volumes found in drive %#X:\n", driveVolume->childNum, drive);
        print("| Sector begin | Sector num | Type |\n");
        SinglyLinkedListNode* current = singlyLinkedListGetNext(&driveVolume->children);
        for (int i = 0; i < driveVolume->childNum; ++i, current = singlyLinkedListGetNext(current)) {
            Volume* v = HOST_POINTER(current, Volume, sibling);
            printf("|  %#010X  | %#010X | %#04X |\n", (Uint32)v->sectorBegin, (Uint32)v->sectorNum, v->flags);
        }
    }

    Volume* bootVolume = volumeGetChild(driveVolume, 0);

    void* kernelEntry = loadKernel(bootVolume, "/boot/kernel.elf");

    if (kernelEntry == NULL) {
        blowup("Not valid kernel!\n");
    }

    enterKernel(kernelEntry, &mMap);

    cli();
    hlt();
    __builtin_unreachable();
}