#include<E820.h>

#include<kit/bit.h>
#include<realmode.h>
#include<stdint.h>
#include<system/memoryMap.h>
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
    MemoryMapEntry buf;
    MemoryMapEntry* table = memoryMap->memoryMapEntries;
    
    initRegs(&registers);

    int cnt = 0;
    registers.ebx = 0;
    do {    //Loop for scanning memory area
        registers.eax = 0xE820;
        registers.ecx = sizeof(MemoryMapEntry);
        registers.edx = SMAP;
        registers.edi = (uint32_t) &buf;

        intInvoke(0x15, &registers, &registers);

        if (TEST_FLAGS(registers.eflags, EFLAGS_CF))
            break;
        
        table[cnt++] = buf;
    } while (registers.ebx != 0 && cnt < MEMORY_AREA_NUM);

    return memoryMap->size = cnt;
}

int detectMemory(SystemInfo* info) {
    int ret = __detectE820(&_memoryMap);
    if (ret == 0) {
        ret = -1;
    }
    info->memoryMap = (uint32_t)&_memoryMap;
    return ret;
}