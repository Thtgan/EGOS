#include<A20.h>
#include<biosIO.h>
#include<blowup.h>
#include<E820.h>
#include<pm.h>
#include<printf.h>
#include<real/simpleAsmLines.h>
#include<system/systemInfo.h>

SystemInfo systemInfo;   //Information about the system

/**
 * @brief Collect information about system hardware, and set the flag of system information
 */
static void __collectSystemInfo();

__attribute__((noreturn))
/**
 * @brief The entrance of the real mode code, initialize properties before switch to protected mode
 * !!(DO NOT CALL THIS FUNCTION)
 */
void realModeMain() {
    __collectSystemInfo();

    //If failed to enable A20, system should not be booted
    if (enableA20()) {
        blowup("Enable A20 failed, unable to boot.\n");
    } else {
        printf("A20 line enabled successfully\n");
    }

    jumpToKernel(&systemInfo);
}

static void __collectSystemInfo() {
    systemInfo.magic = SYSTEM_INFO_MAGIC;
    detectMemory(&systemInfo);
}
