#include<E820.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<realmode.h>
#include<system/memoryMap.h>
#include<system/systemInfo.h>

#define SMAP 0x534D4150

//Memory map to storage memory areas
MemoryMap _memoryMap;

/**
 * @brief Detect memory area with 0x15, EAX = E820
 * 
 * @param memoryMap Memory map
 * @return int number of memory areas detected
 */
static int __E820detect(MemoryMap* memoryMap);

int detectMemory(SystemInfo* info) {
    int ret = __E820detect(&_memoryMap);
    if (ret == 0) {
        ret = -1;
    }
    info->memoryMap = (Uint32)&_memoryMap;
    return ret;
}

static int __E820detect(MemoryMap* memoryMap) {
    MemoryMapEntry buf;
    MemoryMapEntry* table = memoryMap->memoryMapEntries;
    
    initRegs(&registers);

    int cnt = 0;
    registers.ebx = 0;
    do {    //Loop for scanning memory area
        registers.eax = 0xE820;
        registers.ecx = sizeof(MemoryMapEntry);
        registers.edx = SMAP;
        registers.edi = (Uint32) &buf;

        intInvoke(0x15, &registers, &registers);

        if (TEST_FLAGS(registers.eflags, EFLAGS_CF))
            break;
        
        table[cnt++] = buf;
    } while (registers.ebx != 0 && cnt < MEMORY_AREA_NUM);

    memoryMap->entryNum = cnt;

    return cnt;
}