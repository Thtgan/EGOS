#include<GDTInit.h>

#include<realmode.h>
#include<system/GDT.h>

//A basic GDT table
__attribute__((aligned(16)))
static const GDTEntry GDTTable[16] = {
    [GDT_ENTRY_INDEX_NULL] = BUILD_GDT_ENTRY(0x00000000, 0x00000, 0x00, 0x00),
    [GDT_ENTRY_INDEX_KERNEL_CODE] = BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, GDT_ACCESS | GDT_READABLE | GDT_EXECUTABLE | GDT_DESCRIPTOR | GDT_PRIVIEGE_0 | GDT_PRESENT, GDT_GRANULARITY | GDT_SIZE),
    [GDT_ENTRY_INDEX_KERNEL_DATA] = BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, GDT_ACCESS | GDT_READABLE_AND_WRITABLE | GDT_DESCRIPTOR | GDT_PRIVIEGE_0 | GDT_PRESENT, GDT_GRANULARITY | GDT_SIZE),
};

static GDTDesc32 gdtDesc;

GDTDesc32* setupGDT() {
    gdtDesc.size    = (uint16_t)sizeof(GDTTable) - 1;
    gdtDesc.table   = (uint32_t)GDTTable + (getDS() << 4);

    asm volatile("lgdt %0" : : "m" (gdtDesc));

    return &gdtDesc;
}