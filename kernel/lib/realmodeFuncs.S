#include<kit/asm.h>
#include<lib/carrier.h>

ASM_PUBLIC_SYMBOL(realmodeFuncs_begin):

.code16
ASM_FUNC_BEGIN(realmodeFuncs_test):
    mov $0x114514, %eax
    mov $0x1919810, %ebx
    lret
ASM_FUNC_END(realmodeFuncs_test)

ASM_PUBLIC_SYMBOL(realmodeFuncs_end):

CARRIER_MOV_LIST(realmodeFuncs_carryList)