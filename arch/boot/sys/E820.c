#include<sys/E820.h>

#include<bootKit.h>
#include<kit/bit.h>
#include<sys/realmode.h>

#define SMAP 0x534D4150

// LINK src/arch/boot/entry.c#arch_boot_entry_c_bootInfo
extern struct BootInfo bootInfo;

static int __detectE820() {
    struct registerSet regs;
    struct E820Entry buf;
    struct E820Entry* table = bootInfo.memoryMap.e820Table;
    
    initRegs(&regs);

    int cnt = 0;
    regs.ebx = 0;
    do {
        regs.eax = 0xE820;
        regs.ecx = sizeof(struct E820Entry);
        regs.edx = SMAP;
        regs.edi = (uint32_t) &buf;

        intInvoke(0x15, &regs, &regs);

        if(BIT_TEST_FLAGS(regs.eflags, EFLAGS_CF))
            break;
        
        table[cnt++] = buf;
    } while (regs.ebx != 0 && cnt < E820_ENTRY_NUM);

    return bootInfo.memoryMap.e820TableSize = cnt;
}

int detectMemory() {
    int ret = __detectE820();
    if (ret == 0)
        ret = -1;
    return ret;
}