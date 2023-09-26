#include<interrupt/TSS.h>

#include<kernel.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<system/GDT.h>
#include<system/TSS.h>

static TSS _tss;

Result initTSS() {
    memset(&_tss, 0, sizeof(TSS));
    _tss.ist[0] = (Uintptr)pageAlloc(4, MEMORY_TYPE_PUBLIC);
    _tss.rsp[0] = (Uintptr)pageAlloc(4, MEMORY_TYPE_PUBLIC);
    _tss.ioMapBaseAddress = 0x8000;  //Invalid
    
    GDTDesc64* desc = (GDTDesc64*)convertAddressP2V(sysInfo->gdtDesc);
    GDTEntryTSS_LDT* gdtEntryTSS = (GDTEntryTSS_LDT*)((GDTEntry*)desc->table + GDT_ENTRY_INDEX_TSS);

    *gdtEntryTSS = BUILD_GDT_ENTRY_TSS_LDT(((Uintptr)&_tss), sizeof(TSS), GDT_TSS_LDT_TSS | GDT_TSS_LDT_PRIVIEGE_0 | GDT_TSS_LDT_PRESENT, 0);

    asm volatile("ltr %w0" :: "r"(SEGMENT_TSS));

    return RESULT_SUCCESS;
}