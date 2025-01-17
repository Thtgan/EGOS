#include<interrupt/TSS.h>

#include<kernel.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<system/GDT.h>
#include<system/TSS.h>
#include<system/pageTable.h>
#include<result.h>

static TSS _tss; //TODO: Capsule this

Result* tss_init() {
    memory_memset(&_tss, 0, sizeof(TSS));
    _tss.ist[0] = (Uintptr)paging_convertAddressP2V(memory_allocateFrame(4) + 4 * PAGE_SIZE);
    _tss.ist[1] = (Uintptr)paging_convertAddressP2V(memory_allocateFrame(4) + 4 * PAGE_SIZE);
    _tss.rsp[0] = (Uintptr)paging_convertAddressP2V(memory_allocateFrame(4) + 4 * PAGE_SIZE);

    _tss.ioMapBaseAddress = 0x8000;  //Invalid
    
    GDTDesc64* desc = (GDTDesc64*)paging_convertAddressP2V(sysInfo->gdtDesc);
    GDTEntryTSS_LDT* gdtEntryTSS = (GDTEntryTSS_LDT*)((GDTEntry*)desc->table + GDT_ENTRY_INDEX_TSS);

    *gdtEntryTSS = BUILD_GDT_ENTRY_TSS_LDT(((Uintptr)&_tss), sizeof(TSS), GDT_TSS_LDT_TSS | GDT_TSS_LDT_PRIVIEGE_0 | GDT_TSS_LDT_PRESENT, 0);

    asm volatile("ltr %w0" :: "r"(SEGMENT_TSS));

    ERROR_RETURN_OK();
}