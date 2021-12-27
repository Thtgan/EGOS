#include<bootKit.h>
#include<system/address.h>

__attribute__((section("sysInfo")))
SystemInfo systemInfo;   //Information about the system

/**
 * @brief Collect information about system hardware, and set the flag of system information
 */
void collectSystemInfo() {
    detectMemory(&systemInfo);
    systemInfo.flag = SYSTEM_INFO_FLAG;
}

__attribute__((noreturn))
/**
 * @brief The entrance of the real mode code, initialize properties before switch to protected mode
 * !!(DO NOT CALL THIS FUNCTION)
 */
void realModeMain() {
    collectSystemInfo();

    //If failed to enable A20, system should not be booted
    if (enableA20())
        blowup("Enable A20 failed, unable to boot.\n");
    else
        printf("A20 line enabled successfully\n");

    switchToProtectedMode(KERNEL_PHYSICAL_BEGIN);
}