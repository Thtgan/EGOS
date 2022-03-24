#if !defined(__PORTS_HDC_H)
#define __PORTS_HDC_H

#include<kit/bit.h>

//HDC (Hard Disk Controller)

#define HDC_CANNNEL_1                                   0x01F0
#define HDC_CANNNEL_2                                   0x0170

#define HDC_DATA(__CHANNEL)                             (__CHANNEL)
#define HDC_ERROR(__CHANNEL)                            (__CHANNEL + 1)
#define HDC_SECTOR_COUNT(__CHANNEL)                     (__CHANNEL + 2)
#define HDC_LBA_0_7(__CHANNEL)                          (__CHANNEL + 3)
#define HDC_LBA_8_15(__CHANNEL)                         (__CHANNEL + 4)
#define HDC_LBA_16_23(__CHANNEL)                        (__CHANNEL + 5)
#define HDC_LBA_24_27_DEVICE_SELECT(__CHANNEL)          (__CHANNEL + 6)
#define HDC_STATUS(__CHANNEL)                           (__CHANNEL + 7) //Read only
#define HDC_COMMAND(__CHANNEL)                          (__CHANNEL + 7)
#define HDC_CONTROL(__CHANNEL)                          (__CHANNEL + 0x206) //What?
#define HDC_ALT_STATUS(__CHANNEL)                       (__CHANNEL + 0x206) //What?

//No bit definitions for errors, they got different definition for EACH command, it's too much

#define HDC_DEVICE_SELECT_BASE                          0b10100000
#define HDC_SECTOR_SELECT_LBA                           FLAG8(6)
#define HDC_HEAD_SELECT_EXTRACT_HEAD(__HEAD_DRIVE)      EXTRACT_VAL(__HEAD_DRIVE, 8, 0, 4)
#define HDC_DEVICE_SELECT_SLAVE                         FLAG8(4)    //Drive 1 (0) selected

#define HDC_STATUS_ERROR                                FLAG8(0)
#define HDC_STATUS_DATA_REQUIRE_SERVICE                 FLAG8(3)
#define HDC_STATUS_READY                                FLAG8(6)
#define HDC_STATUS_BUSY                                 FLAG8(7)

//Reference: http://https://wiki.osdev.org/ATA_Command_Matrix
//!Might be outdated, some commands may not be available any more
//Duplicated command ignored

#define HDC_COMMAND_NOP                                 0x00
#define HDC_COMMAND_CFA_REQUEST_EXT_ERR_CODE            0x03
#define HDC_COMMAND_DEVICE_RESET                        0x08
#define HDC_COMMAND_READ_SECTORS                        0x20
#define HDC_COMMAND_WRITE_SECTORS                       0x30
#define HDC_COMMAND_CFA_WRITE_SECTORS_NO_ERASE          0x38
#define HDC_COMMAND_READ_VERIFY_SECTORS                 0x40
#define HDC_COMMAND_SEEK                                0x70
#define HDC_COMMAND_CFA_TRANSLATE_SECTOR                0x87
#define HDC_COMMAND_EXECUTE_SEVICE_DIAGNOSTIC           0x90
#define HDC_COMMAND_INIT_DEVICE_PARAMS                  0x91
#define HDC_COMMAND_DOWNLOAD_MICROCODE                  0x92
#define HDC_COMMAND_PACKET                              0xA0
#define HDC_COMMAND_IDENTIFY_PACKET_DEVICE              0xA1
#define HDC_COMMAND_SERVICE                             0xA2
#define HDC_COMMAND_SMART                               0xB0 //Self-Monitoring, Analysis, and Reporting Technology
#define HDC_COMMAND_CFA_ERASE_SECTORS                   0xC0
#define HDC_COMMAND_READ_MULTIPLE                       0xC4
#define HDC_COMMAND_WRITE_MULTIPLE                      0xC5
#define HDC_COMMAND_SET_MULTIPLE_MODE                   0xC6
#define HDC_COMMAND_READ_DMA_QUEUED                     0xC7
#define HDC_COMMAND_READ_DMA                            0xC8
#define HDC_COMMAND_WRITE_DMA                           0xCA
#define HDC_COMMAND_WRITE_QUEUED                        0xCC
#define HDC_COMMAND_WRITE_MULTIPLE_NO_ERASE             0xCD
#define HDC_COMMAND_CFA_WRITE_MULTIPLE_NO_ERASE         0xCE
#define HDC_COMMAND_CHECK_MEDIA_CARD_TYPE               0xD1
#define HDC_COMMAND_GET_MEDIA_STATUS                    0xDA
#define HDC_COMMAND_MEDIA_LOCK                          0xDE
#define HDC_COMMAND_MEDIA_UNLOCK                        0xDF
#define HDC_COMMAND_STANDBY_IMMEDIATE                   0xE0
#define HDC_COMMAND_IDLE_IMMEDIATE                      0xE1
#define HDC_COMMAND_STANDBY                             0xE2
#define HDC_COMMAND_IDLE                                0xE3
#define HDC_COMMAND_READ_BUFFER                         0xE4
#define HDC_COMMAND_CHECK_POWER_MODE                    0xE5
#define HDC_COMMAND_SLEEP                               0xE6
#define HDC_COMMAND_FLUSH_CACHE                         0xE7
#define HDC_COMMAND_WRITE_BUFFER                        0xE8
#define HDC_COMMAND_IDENTIFY_DEVICE                     0xEC
#define HDC_COMMAND_MEDIA_EJECT                         0xED
#define HDC_COMMAND_SET_FEATURES                        0xEF
#define HDC_COMMAND_SET_PASSWORD                        0xF1
#define HDC_COMMAND_UNLOCK                              0xF2
#define HDC_COMMAND_ERASE_PREPARE                       0xF3
#define HDC_COMMAND_ERASE_UNIT                          0xF4
#define HDC_COMMAND_ERASE_LOCK                          0xF5
#define HDC_COMMAND_DISABLE_PASSWORD                    0xF6
#define HDC_COMMAND_READ_NATIVE_MAX_ADDR                0xF8
#define HDC_COMMAND_WRITE_MAX_ADDR                      0xF9

#endif // __PORTS_HDC_H
