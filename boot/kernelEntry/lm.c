#include<lm.h>

#include<kit/types.h>
#include<system/address.h>
#include<system/GDT.h>
#include<system/systemInfo.h>

GDTDesc64 gdtDesc64;

__attribute__((noreturn))
extern void __jumpToLongMode(Uint16 codeSegment, Uint32 kernelAddr, Uint32 sysInfo);

__attribute__((noreturn))
void jumpToLongMode(Uint32 sysInfo) {
    SystemInfo* info = (SystemInfo*)sysInfo;
    GDTDesc32* desc32 = (GDTDesc32*)(Uint32)info->gdtDesc;
    GDTEntry* table = (GDTEntry*)desc32->table;
    //In 64-bit mode, the processor DOES NOT perform runtime limit checking on code or data segments. However, the processor does check descriptor-table limits.
    //That's why we can jump to kernel starts at position far above segment's limitation
    //<<Intel® 64 and IA-32 Architectures Software Developer’s Manual>> 5.3.1
    table[GDT_ENTRY_INDEX_KERNEL_CODE] = BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, GDT_ACCESS | GDT_READABLE | GDT_EXECUTABLE | GDT_DESCRIPTOR | GDT_PRIVIEGE_0 | GDT_PRESENT, GDT_GRANULARITY | GDT_LONG_MODE);
    table[GDT_ENTRY_INDEX_KERNEL_DATA] = BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, GDT_ACCESS | GDT_READABLE_AND_WRITABLE | GDT_DESCRIPTOR | GDT_PRIVIEGE_0 | GDT_PRESENT, GDT_GRANULARITY | GDT_LONG_MODE);
    table[GDT_ENTRY_INDEX_USER_CODE32] = BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, GDT_ACCESS | GDT_READABLE | GDT_EXECUTABLE | GDT_DESCRIPTOR | GDT_PRIVIEGE_3 | GDT_PRESENT, GDT_GRANULARITY | GDT_SIZE);
    table[GDT_ENTRY_INDEX_USER_DATA] = BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, GDT_ACCESS | GDT_READABLE_AND_WRITABLE | GDT_DESCRIPTOR | GDT_PRIVIEGE_3 | GDT_PRESENT, GDT_GRANULARITY | GDT_LONG_MODE);
    table[GDT_ENTRY_INDEX_USER_CODE] = BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, GDT_ACCESS | GDT_READABLE | GDT_EXECUTABLE | GDT_DESCRIPTOR | GDT_PRIVIEGE_3 | GDT_PRESENT, GDT_GRANULARITY | GDT_LONG_MODE);

    gdtDesc64.size = desc32->size;
    gdtDesc64.table = desc32->table;

    info->gdtDesc = (Uint32)&gdtDesc64;

    asm volatile("lgdt %0" : : "m" (gdtDesc64));

    __jumpToLongMode(SEGMENT_KERNEL_CODE, KERNEL_PHYSICAL_BEGIN, sysInfo);
}