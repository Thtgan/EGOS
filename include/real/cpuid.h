#if !defined(__CPUID_H)
#define __CPUID_H

#include<kit/bit.h>
#include<real/flags/eflags.h>
#include<real/simpleAsmLines.h>
#include<stdbool.h>
#include<stdint.h>

#define CPUID_GET_HIGHEST_BASIC_FUNCTION_AND_MANUFACTURER_ID 0x0

#define CPUID_INFO_AND_FEATURE 0x1

#define CPUID_CACHE_AND_TLB_DESCRIPTION_INFO 0x2

#define CPUID_PROCESSOR_SERIAL_NUMBER 0x3

#define CPUID_INTEL_CORE_CACHE_TOPOLOGY 0x4, 0xB

#define CPUID_THERMAL_AND_POWER_MANAGEMENT 0x6

#define CPUID_EXTENDED_FEATURED1 0x7, 0x0

#define CPUID_EXTENDED_FEATURED2 0x7, 0x1

#define CPUID_GET_HIGHEST_EXTENDED_FUNCTION 0x80000000

#define CPUID_EXTENDED_INFO_AND_FEATURE 0x80000001

#define CPUID_BRAND_STRING1 0x80000002

#define CPUID_BRAND_STRING2 0x80000003

#define CPUID_BRAND_STRING3 0x80000004

#define CPUID_L1_CACHE_AND_TLB_IDENTIFIERS 0x80000005

#define CPUID_EXTENDED_L2_CACHE_FEATURES 0x80000006

#define CPUID_ADVANCED_POWER_MANAGEMENT_INFO 0x80000007

#define CPUID_VIRTUAL_AND_PHYSICAL_ADDRESS_SIZES 0x80000008

//TODO: Fill flag macros

//AuthenticAMD
#define CPUID_SIGNATURE_AMD_EBX         0x68747541
#define CPUID_SIGNATURE_AMD_ECX         0x444D4163
#define CPUID_SIGNATURE_AMD_EDX         0x69746E65

//CentaurHauls
#define CPUID_SIGNATURE_CENTAUR_EBX     0x746E6543
#define CPUID_SIGNATURE_CENTAUR_ECX     0x736C7561
#define CPUID_SIGNATURE_CENTAUR_EDX     0x48727561

//CyrixInstead
#define CPUID_SIGNATURE_CYRIX_EBX       0x69727943
#define CPUID_SIGNATURE_CYRIX_ECX       0x64616574
#define CPUID_SIGNATURE_CYRIX_EDX       0x736E4978

//GenuineIntel
#define CPUID_SIGNATURE_INTEL_EBX       0x756E6547
#define CPUID_SIGNATURE_INTEL_ECX       0x6C65746E
#define CPUID_SIGNATURE_INTEL_EDX       0x49656E69

//TransmetaCPU
#define CPUID_SIGNATURE_TRANSMETA1_EBX  0x6E617254
#define CPUID_SIGNATURE_TRANSMETA1_ECX  0x55504361
#define CPUID_SIGNATURE_TRANSMETA1_EDX  0x74656D73

//GenuineTMx86
#define CPUID_SIGNATURE_TRANSMETA2_EBX  0x756E6547
#define CPUID_SIGNATURE_TRANSMETA2_ECX  0x3638784D
#define CPUID_SIGNATURE_TRANSMETA2_EDX  0x54656E69

//Geode by NSC
#define CPUID_SIGNATURE_NSC_EBX         0x646F6547
#define CPUID_SIGNATURE_NSC_ECX         0x43534E20
#define CPUID_SIGNATURE_NSC_EDX         0x79622065

//NexGenDriven
#define CPUID_SIGNATURE_NEXT_GEN_EBX    0x4778654E
#define CPUID_SIGNATURE_NEXT_GEN_ECX    0x6E657669
#define CPUID_SIGNATURE_NEXT_GEN_EDX    0x72446E65

//RiseRiseRise
#define CPUID_SIGNATURE_RISE_EBX        0x65736952
#define CPUID_SIGNATURE_RISE_ECX        0x65736952
#define CPUID_SIGNATURE_RISE_EDX        0x65736952

//SiS SiS SiS 
#define CPUID_SIGNATURE_SIS_EBX         0x20536953
#define CPUID_SIGNATURE_SIS_ECX         0x20536953
#define CPUID_SIGNATURE_SIS_EDX         0x20536953

//UMC UMC UMC 
#define CPUID_SIGNATURE_UMC_EBX         0x20434D55
#define CPUID_SIGNATURE_UMC_ECX         0x20434D55
#define CPUID_SIGNATURE_UMC_EDX         0x20434D55

//VIA VIA VIA 
#define CPUID_SIGNATURE_VIA_EBX         0x20414956
#define CPUID_SIGNATURE_VIA_ECX         0x20414956
#define CPUID_SIGNATURE_VIA_EDX         0x20414956

//Vortex86 SoC
#define CPUID_SIGNATURE_VORTEX_EBX      0x74726F56
#define CPUID_SIGNATURE_VORTEX_ECX      0x436F5320
#define CPUID_SIGNATURE_VORTEX_EDX      0x36387865

//  Shanghai  
#define CPUID_SIGNATURE_ZHAOXIN_EBX     0x68532020
#define CPUID_SIGNATURE_ZHAOXIN_ECX     0x20206961
#define CPUID_SIGNATURE_ZHAOXIN_EDX     0x68676E61

//HygonGenuine
#define CPUID_SIGNATURE_HYGON_EBX       0x6F677948
#define CPUID_SIGNATURE_HYGON_ECX       0x656E6975
#define CPUID_SIGNATURE_HYGON_EDX       0x6E65476E

static inline bool checkCPUID() {
    pushfl();
    uint32_t eflags = readEFlags32();

    writeEFlags32(REVERSE_FLAG(eflags, EFLAGS_ID));

    eflags ^= readEFlags32();

    popfl();

    return eflags != 0;
}

#define CPUID(__IN_EAX, __OUT_EAX, __OUT_EBX, __OUT_ECX, __OUT_EDX)         \
asm volatile(                                                               \
    "cpuid"                                                                 \
    : "=a"(__OUT_EAX), "=b"(__OUT_EBX), "=c"(__OUT_ECX), "=d"(__OUT_EDX)    \
    : "0"(__IN_EAX)                                                         \
    );

#define CPUID_WITH_SUB_PARAM(__IN_EAX, __IN_ECX, __OUT_EAX, __OUT_EBX, __OUT_ECX, __OUT_EDX)    \
asm volatile(                                                                                   \
    "cpuid"                                                                                     \
    : "=a"(__OUT_EAX), "=b"(__OUT_EBX), "=c"(__OUT_ECX), "=d"(__OUT_EDX)                        \
    : "0"(__IN_EAX), "2"(__IN_ECX)                                                              \
    );

#endif // __CPUID_H
