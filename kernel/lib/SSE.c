#include<lib/SSE.h>

#include<kit/bit.h>
#include<real/cpuid.h>
#include<real/flags/cr0.h>
#include<real/flags/cr4.h>
#include<real/simpleAsmLines.h>
#include<stdbool.h>
#include<stdint.h>

bool checkSSE() {
    uint32_t eax, ebx, ecx, edx;
    CPUID(CPUID_INFO_AND_FEATURE, eax, ebx, ecx, edx);
    return TEST_FLAGS(edx, FLAG32(25));
}

void enableSSE() {
    uint64_t cr0 = readCR0_64();
    SET_FLAG_BACK(cr0, CR0_MP);
    writeCR0_64(cr0);

    uint64_t cr4 = readCR4_64();
    SET_FLAG_BACK(cr4, CR4_OSFXSR | CR4_OSXMMEXCPT);
    writeCR4_64(cr4);
}