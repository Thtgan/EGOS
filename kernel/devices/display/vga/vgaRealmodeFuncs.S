#include<carrier.h>
#include<kit/asm.h>

ASM_PUBLIC_SYMBOL(vgaRealmodeFuncs_begin):

.code16
ASM_FUNC_BEGIN(vgaRealmodeFuncs_int10):
    int $0x10
    lret
ASM_FUNC_END(vgaRealmodeFuncs_int10)

ASM_PUBLIC_SYMBOL(vgaRealmodeFuncs_end):

CARRIER_MOV_LIST(vgaRealmodeFuncs_carryList)