#if !defined(__MSR_H)
#define __MSR_H

#include<kit/bit.h>

//Extended Feature Enable Register
#define MSR_ADDR_EFER   0xC0000080

//System Call Extensions
#define MSR_EFER_FLAG_SCE_INDEX     0
#define MSR_EFER_FLAG_SCE           FLAG32(MSR_EFER_FLAG_SCE_INDEX)

//Long Mode Enable
#define MSR_EFER_FLAG_LME_INDEX     8
#define MSR_EFER_FLAG_LME           FLAG32(MSR_EFER_FLAG_LME_INDEX)

//Long Mode Active
#define MSR_EFER_FLAG_LMA_INDEX     10
#define MSR_EFER_FLAG_LMA           FLAG32(MSR_EFER_FLAG_LMA_INDEX)

//No-Execute Enable
#define MSR_EFER_FLAG_NXE_INDEX     11
#define MSR_EFER_FLAG_NXE           FLAG32(MSR_EFER_FLAG_NXE_INDEX)

//Secure Virtual Machine Enable
#define MSR_EFER_FLAG_SVME_INDEX    12
#define MSR_EFER_FLAG_SVME          FLAG32(MSR_EFER_FLAG_SVME_INDEX)

//Long Mode Segment Limit Enable
#define MSR_EFER_FLAG_LMSLE_INDEX   13
#define MSR_EFER_FLAG_LMSLE         FLAG32(MSR_EFER_FLAG_LMSLE_INDEX)

//Fast FXSAVE/FXRSTOR
#define MSR_EFER_FLAG_FFXSR_INDEX   14
#define MSR_EFER_FLAG_FFXSR         FLAG32(MSR_EFER_FLAG_FFXSR_INDEX)

//Translation Cache Extension
#define MSR_EFER_FLAG_TCE_INDEX     15
#define MSR_EFER_FLAG_TCE           FLAG32(MSR_EFER_FLAG_TCE)

#endif // __MSR_H
