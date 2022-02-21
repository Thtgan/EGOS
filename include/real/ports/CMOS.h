#if !defined(__PORTS_CMOS_H)
#define __PORTS_CMOS_H

#include<kit/bit.h>

//CMOS RAM/RTC

//CMOS register select index port
#define CMOS_INDEX  0x70

#define CMOS_NMI_DISABLE                                    FLAG8(7)    //Non-Maskable Interrupt (not) disabled
#define CMOS_INDEX_SELECT(__INDEX)                          TRIM_VAL_SIMPLE(__INDEX, 8, 7)

//CMOS data port
#define CMOS_DATA                                           0x71

#define CMOS_RTC_CURRENT_SECOND                             0x00
#define CMOS_RTC_ALARM_SECOND                               0x01
#define CMOS_RTC_CURRENT_MINUTE                             0x02
#define CMOS_RTC_ALARM_MINUTE                               0x03
#define CMOS_RTC_CURRENT_HOUR                               0x04
#define CMOS_RTC_ALARM_HOUR                                 0x05
#define CMOS_DAT_OF_WEEK                                    0x06
#define CMOS_DAT_OF_MONTH                                   0x07
#define CMOS_MONTH                                          0x08
#define CMOS_YEAR                                           0x09
#define CMOS_STATUS_A                                       0x0A

#define CMOS_STATUS_A_RATE_SELECTION_FREQUENCY_NONE         0b0000
#define CMOS_STATUS_A_RATE_SELECTION_FREQUENCY_1            0b0011      //122 ms
#define CMOS_STATUS_A_RATE_SELECTION_FREQUENCY_2            0b1111      //500 ms
#define CMOS_STATUS_A_RATE_SELECTION_FREQUENCY_3            0b0110      //976.562 ms
#define CMOS_STATUS_A_TIME_FREQUENCY_DIVIDER_1              0x010       //32.768 kHz
#define CMOS_STATUS_A_RTC_UPGRADING                         FLAG8(7)    //RTC (not) upgrading

#define CMOS_STATUS_B                                       0x0B

#define CMOS_STATUS_B_DAYLIGHT_SAVINGS                      FLAG8(0)    //(not) enable daylight savings
//From reference:
//Only in USA. useless in Europe. Some DOS versions clear this bit when you use the DAT/TIME command.
#define CMOS_STATUS_B_12_HOUR_MODE                          FLAG8(1)    //Clock in 12 (24) hour mode
#define CMOS_STATUS_B_BINARY_CALENDER                       FLAG8(2)    //Data mode in (BCD) binary format
#define CMOS_STATUS_B_SQUARE_WAVE_OUTPUT                    FLAG8(3)    //(not) enable square wave output
#define CMOS_STATUS_B_UPDATE_ENDED_INTERRUPT                FLAG8(4)    //(not) enable update-ended interrupt
#define CMOS_STATUS_B_ALARM_INTERRUPT                       FLAG8(5)    //(not) enable alarm interrupt
#define CMOS_STATUS_B_PERIOSIC_INTERRUPT                    FLAG8(6)    //(not) enable periodic interrupt
#define CMOS_STATUS_B_HALT_CLOCK_UPDATE                     FLAG8(7)    //(not) halt clock update

#define CMOS_STATUS_C 0x0C  //Read only status

#define CMOS_STATUS_C_UPDATE_ENDED_INTERRUPT                FLAG8(4)    //update-ended interrupt flag
#define CMOS_STATUS_C_ALARM_INTERRUPT                       FLAG8(5)    //alarm interrupt flag
#define CMOS_STATUS_C_PERIOSIC_INTERRUPT                    FLAG8(6)    //periodic interrupt flag
//When any or all of bits 6-4 are 1 and appropriate enables (Register B) are set to 1. Generates IRQ 8 when triggered
#define CMOS_STATUS_C_INTERRUPT_REQUEST                     FLAG8(7)

#define CMOS_STATUS_D                                       0x0D
#define CMOS_STATUS_D_VALID_MEMORY                          FLAG8(7)    //CMOS battery good (bad)

#define CMOS_DIAGNOSTIC_STATUS                              0x0E
#define CMOS_DIAGNOSTIC_STATUS_READING_ADAPTER_ID_TIMEOUT   FLAG8(0)
#define CMOS_DIAGNOSTIC_STATUS_ADAPTER_NOT_MATCH_CONFIG     FLAG8(1)
#define CMOS_DIAGNOSTIC_STATUS_TIME_INVALID                 FLAG8(2)
#define CMOS_DIAGNOSTIC_STATUS_DISK_ADAPTER_INIT_FAILED     FLAG8(3)
#define CMOS_DIAGNOSTIC_STATUS_MEMORY_SIZE_ERROR            FLAG8(4)
#define CMOS_DIAGNOSTIC_STATUS_POST_CONFIG_INFO_INVALID     FLAG8(5)
#define CMOS_DIAGNOSTIC_STATUS_BAD_CHECKSUM                 FLAG8(6)
#define CMOS_DIAGNOSTIC_RTC_POWER_LOST                      FLAG8(7)

#define CMOS_SHUTDOWN_STATUS                                0x0F
#define CMOS_SHUTDOWN_STATUS_POWER_ON                       0x00
#define CMOS_SHUTDOWN_STATUS_MEMORY_SIZE_PASSED             0x01
#define CMOS_SHUTDOWN_STATUS_MEMORY_TEST_PASSED             0x02
#define CMOS_SHUTDOWN_STATUS_MEMORY_TEST_FAILED             0x03
#define CMOS_SHUTDOWN_STATUS_BOOTSTRAP                      0x04
#define CMOS_SHUTDOWN_STATUS_JUMP_DWORD_POINTER_EOI         0x05
#define CMOS_SHUTDOWN_STATUS_PROTECTED_MODE_TEST_PASSED     0x06
#define CMOS_SHUTDOWN_STATUS_PROTECTED_MODE_TEST_FAILED     0x07
#define CMOS_SHUTDOWN_STATUS_MEMORY_SIZE_FAILED             0x08
#define CMOS_SHUTDOWN_STATUS_INT15_BLOCK_MOVE               0x09
#define CMOS_SHUTDOWN_STATUS_JUMP_DWORD_POINTER_NO_EOI      0x0A

#endif // __PORTS_CMOS_H
