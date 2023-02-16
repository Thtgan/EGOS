#include<pm.h>

#include<GDTInit.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<realmode.h>
#include<real/flags/cr0.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>
#include<system/systemInfo.h>

__attribute__((noreturn)) //Use register to pass parameters for better stack operations
/**
 * @brief Do necessary initializations and jump to protected mode kernel
 * 
 * @see arch/boot/jmp2pmCode.asm
 * 
 * @param codeSegment Code segment
 * @param dataSegment Data segment
 * @param protectedBegin The address where the kernel will be loaded to
 */
extern void jumpToProtectedMode(uint16_t codeSegment, uint16_t dataSegment, uint32_t sysInfo);

__attribute__((noreturn))
void jumpToKernel(SystemInfo* sysInfo) {
    sysInfo->gdtDesc = (uint32_t)setupGDT();
    
    //Switch to protected mode
    writeRegister_CR0_32(SET_FLAG(readRegister_CR0_32(), CR0_PE));

    //ANCHOR[id=arch_boot_sys_pm_c_cli]
    cli();//Clear interrupt flag

    jumpToProtectedMode(SEGMENT_KERNEL_CODE, SEGMENT_KERNEL_DATA, (uint32_t)sysInfo);
}