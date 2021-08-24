#include<sys/GDT.h>

#include<sys/registers.h>

static const struct GDTEntry GDTTable[] __attribute__((aligned(16))) = {
    [GDT_ENTRY_INDEX_NULL] = BUILD_GDT_ENTRY(0x00000000, 0x00000, 0x00, 0x00),
    [GDT_ENTRY_INDEX_CODE] = BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, 0x9B, 0x0C),
    [GDT_ENTRY_INDEX_DATA] = BUILD_GDT_ENTRY(0x00000000, 0xFFFFF, 0x93, 0x0C),
};

static struct GDTDesc gdtDesc;

void setGDT() {
    gdtDesc.GDTTableSize = sizeof(GDTTable) - 1;
    gdtDesc.GDTTablePtr = (uint32_t)GDTTable + (getDS() << 4);

    asm volatile("lgdtl %0" : : "m" (gdtDesc));
}