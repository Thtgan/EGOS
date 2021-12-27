#include<sys/E820.h>

#include<bootKit.h>
#include<kit/bit.h>
#include<sys/realmode.h>
#include<system/memoryArea.h>
#include<system/systemInfo.h>

#define SMAP 0x534D4150

//Memory map to storage memory areas
MemoryMap _memoryMap;

/**
 * @brief Detect memory area with 0x15, EAX = E820
 * 
 * @return number of memory areas detected
 */
static int __detectE820(MemoryMap* memoryMap) {
    MemoryAreaEntry buf;
    MemoryAreaEntry* table = memoryMap->memoryAreas;
    
    initRegs(&registers);

    int cnt = 0;
    registers.ebx = 0;
    do {    //Loop for scan memory area
        registers.eax = 0xE820;
        registers.ecx = sizeof(MemoryAreaEntry);
        registers.edx = SMAP;
        registers.edi = (uint32_t) &buf;

        intInvoke(0x15, &registers, &registers);

        if(TEST_FLAGS(registers.eflags, EFLAGS_CF))
            break;
        
        table[cnt++] = buf;
    } while (registers.ebx != 0 && cnt < MEMORY_AREA_NUM);

    return memoryMap->size = cnt;
}

int detectMemory(SystemInfo* info) {
    int ret = __detectE820(&_memoryMap);
    if (ret == 0)
        ret = -1;
    info->memoryMap = &_memoryMap;
    return ret;
}