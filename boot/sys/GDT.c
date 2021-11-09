#include<sys/GDT.h>

#include<sys/realmode.h>

//A basic GDT table
__attribute__((aligned(16)))
static const struct GDTEntry GDTTable[256] = {
    [GDT_ENTRY_INDEX_NULL]  =   BUILD_GDT_ENTRY(0x00000000, 0x00000, 0x00, 0x00),
    [GDT_ENTRY_INDEX_CODE]  =   BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, GDT_ACCESS | GDT_READABLE | GDT_EXECUTABLE | GDT_DESCRIPTOR | GDT_PRIVIEGE_0 | GDT_PRESENT, GDT_GRANULARITY | GDT_SIZE),
    [GDT_ENTRY_INDEX_DATA]  =   BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, GDT_ACCESS | GDT_READABLE_AND_WRITABLE | GDT_DESCRIPTOR | GDT_PRIVIEGE_0 | GDT_PRESENT, GDT_GRANULARITY | GDT_SIZE),
};

static struct GDTDesc gdtDesc;

void setupGDT() {
    gdtDesc.size        = (uint16_t)sizeof(GDTTable) - 1;
    gdtDesc.tablePtr    = (uint32_t)GDTTable + (getDS() << 4);

    asm volatile("lgdt %0" : : "m" (gdtDesc));
}