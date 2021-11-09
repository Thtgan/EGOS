#include<sys/E820.h>

#include<bootKit.h>
#include<kit/bit.h>
#include<sys/realmode.h>
#include<system/memoryArea.h>
#include<system/systemInfo.h>

#define SMAP 0x534D4150

//Memory map to storage memory areas
struct MemoryMap _memoryMap;

/**
 * @brief Detect memory area with 0x15, EAX = E820
 * 
 * @return number of memory areas detected
 */
static int __detectE820(struct MemoryMap* memoryMap) {
    struct registerSet regs;
    struct MemoryAreaEntry buf;
    struct MemoryAreaEntry* table = memoryMap->memoryAreas;
    
    initRegs(&regs);

    int cnt = 0;
    regs.ebx = 0;
    do {    //Loop for scan memory area
        regs.eax = 0xE820;
        regs.ecx = sizeof(struct MemoryAreaEntry);
        regs.edx = SMAP;
        regs.edi = (uint32_t) &buf;

        intInvoke(0x15, &regs, &regs);

        if(BIT_TEST_FLAGS(regs.eflags, EFLAGS_CF))
            break;
        
        table[cnt++] = buf;
    } while (regs.ebx != 0 && cnt < MEMORY_AREA_NUM);

    return memoryMap->size = cnt;
}

int detectMemory(struct SystemInfo* info) {
    int ret = __detectE820(&_memoryMap);
    if (ret == 0)
        ret = -1;
    info->memoryMap = &_memoryMap;
    return ret;
}