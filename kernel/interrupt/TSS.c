#include<interrupt/TSS.h>

#include<kernel.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/GDT.h>
#include<system/TSS.h>
#include<system/pageTable.h>

static TSS _tss; //TODO: Capsule this

void tss_init() {
    memory_memset(&_tss, 0, sizeof(TSS));
    _tss.ist[0] = (Uintptr)PAGING_CONVERT_KERNEL_MEMORY_P2V(mm_allocateFrames(4) + 4 * PAGE_SIZE);  //TODO: Identical mapping should not live so long
    _tss.ist[1] = (Uintptr)PAGING_CONVERT_KERNEL_MEMORY_P2V(mm_allocateFrames(4) + 4 * PAGE_SIZE);
    _tss.rsp[0] = (Uintptr)PAGING_CONVERT_KERNEL_MEMORY_P2V(mm_allocateFrames(4) + 4 * PAGE_SIZE);

    _tss.ioMapBaseAddress = 0x8000;  //Invalid
    
    GDTDesc64* desc = (GDTDesc64*)PAGING_CONVERT_KERNEL_MEMORY_P2V(sysInfo->gdtDesc);
    GDTEntryTSS_LDT* gdtEntryTSS = (GDTEntryTSS_LDT*)((GDTEntry*)desc->table + GDT_ENTRY_INDEX_TSS);

    *gdtEntryTSS = BUILD_GDT_ENTRY_TSS_LDT(((Uintptr)&_tss), sizeof(TSS), GDT_TSS_LDT_TSS | GDT_TSS_LDT_PRIVIEGE_0 | GDT_TSS_LDT_PRESENT, 0);

    asm volatile("ltr %w0" :: "r"(SEGMENT_TSS));
}