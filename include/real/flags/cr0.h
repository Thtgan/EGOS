#if !defined(__REAL_FLAGS_CR0_H)
#define __REAL_FLAGS_CR0_H

#include<kit/bit.h>

//Reference: https://wiki.osdev.org/Control_Register#Control_Registers

//Protected Mode Enable
#define CR0_PE_INDEX    0
#define CR0_PE          FLAG32(CR0_PE_INDEX)

//Monitor co-processor
#define CR0_MP_INDEX    1
#define CR0_MP          FLAG32(CR0_MP_INDEX)

//x87 FPU Emulation
#define CR0_EM_INDEX    2
#define CR0_EM          FLAG32(CR0_EM_INDEX)

//Task switched
#define CR0_TS_INDEX    3
#define CR0_TS          FLAG32(CR0_TS_INDEX)

//Extension type
#define CR0_ET_INDEX    4
#define CR0_ET          FLAG32(CR0_ET_INDEX)

//Numeric error
#define CR0_NE_INDEX    5
#define CR0_NE          FLAG32(CR0_NE_INDEX)

//Write protect
#define CR0_WP_INDEX    16
#define CR0_WP          FLAG32(CR0_WP_INDEX)

//Alignment mask
#define CR0_AM_INDEX    18
#define CR0_AM          FLAG32(CR0_AM_INDEX)

//Not-write through
#define CR0_NW_INDEX    29
#define CR0_NW          FLAG32(CR0_NW_INDEX)

//Cache disable
#define CR0_CD_INDEX    30
#define CR0_CD          FLAG32(CR0_CD_INDEX)

//Paging
#define CR0_PG_INDEX    31
#define CR0_PG          FLAG32(CR0_PG_INDEX)

#endif // __REAL_FLAGS_CR0_H
