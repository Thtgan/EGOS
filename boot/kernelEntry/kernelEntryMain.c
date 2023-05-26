#include<kit/bit.h>
#include<kit/types.h>
#include<lm.h>
#include<pagingInit.h>
#include<real/cpuid.h>
#include<real/simpleAsmLines.h>
#include<system/address.h>
#include<system/GDT.h>
#include<system/systemInfo.h>

/**
 * @brief Check if long mode is supported
 * 
 * @return bool Is long mode supported
 */
static inline bool __checkLongMode();

__attribute__((noreturn))
void kernelEntry(Uint32 magic, Uint32 sysInfo) {
    SystemInfo* info = (SystemInfo*)sysInfo;
    if (!checkCPUID() || !__checkLongMode()) {
        die();
    }

    initPaging(info);
    //Compatibility mode from now on

    jumpToLongMode(sysInfo);
}

static inline bool __checkLongMode() {
    Uint32 eax, ebx, ecx, edx;

    CPUID(CPUID_GET_HIGHEST_EXTENDED_FUNCTION, eax, ebx, ecx, edx);

    if (eax < CPUID_EXTENDED_INFO_AND_FEATURE) {
        return false;
    }

    CPUID(CPUID_EXTENDED_INFO_AND_FEATURE, eax, ebx, ecx, edx);

    if (TEST_FLAGS_NONE(edx, 1 << 29)) { //TODO: Replace with flag macro
        return false;
    }

    return true;
}

