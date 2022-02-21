#if !defined(__PORTS_HDC_H)
#define __PORTS_HDC_H

#include<kit/bit.h>

//HDC (Hard Disk Controller)

#define HDC_DATA_CHANNEL_1                          0x01F0
#define HDC_ERROR_CHANNEL_1                         0x01F1
#define HDC_SECTOR_COUNT_CHANNEL_1                  0x01F2
#define HDC_SECTOR_NUMBER_CHANNEL_1                 0x01F3
#define HDC_CYLINDER_LOW_CHANNEL_1                  0x01F4
#define HDC_CYLINDER_HIGH_CHANNEL_1                 0x01F5
#define HDC_DRIVE_HEAD_CHANNEL_1                    0x01F6
#define HDC_STATUS_CHANNEL_1                        0x01F7  //Read only
#define HDC_COMMAND_CHANNEL_1                       0x01F8


#define HDC_DATA_CHANNEL_2                          0x0170
#define HDC_ERROR_CHANNEL_2                         0x0171
#define HDC_SECTOR_COUNT_CHANNEL_2                  0x0172
#define HDC_SECTOR_NUMBER_CHANNEL_2                 0x0173
#define HDC_CYLINDER_LOW_CHANNEL_2                  0x0174
#define HDC_CYLINDER_HIGH_CHANNEL_2                 0x0175
#define HDC_DRIVE_HEAD_CHANNEL_2                    0x0176
#define HDC_STATUS_CHANNEL_2                        0x0177
#define HDC_COMMAND_CHANNEL_2                       0x0178

#define HDC_ERROR_DIAGNOSTIC_EXTRACT_ERROR(__ERROR) EXTRACT_VAL(__ERROR, 8, 0, 3)
#define HDC_ERROR_DIAGNOSTIC_NO_ERROR               0b001
#define HDC_ERROR_DIAGNOSTIC_FORMATTER_DEVICE_ERROR 0b010
#define HDC_ERROR_DIAGNOSTIC_SECTOR_BUFFER_ERROR    0b011
#define HDC_ERROR_DIAGNOSTIC_ECC_CIRCUITRY_ERROR    0b100
#define HDC_ERROR_DIAGNOSTIC_MICROPROCESSOR_ERROR   0b101

#define HDC_DRIVE_HEAD_EXTRACT_HEAD(__HEAD_DRIVE)   EXTRACT_VAL(__HEAD_DRIVE, 8, 0, 4)
#define HDC_DRIVE_HEAD_DRIVE_SELECTION              FLAG8(4)    //Drive 1 (0) selected

#define HDC_STATUS_PREVIOUS_COMMAND_ENDED_IN_ERROR  FLAG8(0)
#define HDC_STATUS_DISK_REVOLUTION_SET_TO_1         FLAG8(1)
#define HDC_STATUS_DISK_READ_CORRECTED              FLAG8(2)
#define HDC_STATUS_SECTOR_BUFFER_REQUIRE_SERVICING  FLAG8(3)
#define HDC_STATUS_SEEK_COMPLETE                    FLAG8(4)
#define HDC_STATUS_WRITE_FAULT                      FLAG8(5)
#define HDC_STATUS_DRIVE_READY                      FLAG8(6)
#define HDC_STATUS_EXECUTING                        FLAG8(7)

#define HDC_COMMAND_NOP                             0x00
#define HDC_COMMAND_CFA_REQUEST_EXT_ERR_CODE        0x03
#define HDC_COMMAND_DATA_SET_MEMAGEMENT             0x06
#define HDC_COMMAND_DATA_SET_MEMAGEMENT_XL          0x07
#define HDC_COMMAND_REQUEST_SENSE_DATA_EXT          0x0B
#define HDC_COMMAND_GET_PHYSICAL_ELEMENT_STATUS     0x12
#define HDC_COMMAND_READ_SECTORS                    0x20
#define HDC_COMMAND_READ_SECTORS_EXT                0x24
#define HDC_COMMAND_READ_DMA_EXT                    0x25
#define HDC_COMMAND_READ_STREAM_DMA_EXT             0x2A
#define HDC_COMMAND_READ_STREAM_EXT                 0x2B
#define HDC_COMMAND_READ_LOG_EXT                    0x2F
#define HDC_COMMAND_WRITE_SECTORS                   0x30
#define HDC_COMMAND_WRITE_SECTORS_EXT               0x34
#define HDC_COMMAND_WRITE_DMA_EXT                   0x35
#define HDC_COMMAND_CFA_WRITE_SECTORS_NO_ERASE      0x38
#define HDC_COMMAND_WRITE_STREAM_DMA_EXT            0x3A
#define HDC_COMMAND_WRITE_STREAM_EXT                0x3B
#define HDC_COMMAND_WRITE_DMA_FUA_EXT               0x3D
#define HDC_COMMAND_WRITE_LOG_EXT                   0x3F
#define HDC_COMMAND_READ_VERIFY_SECTORS             0x40
#define HDC_COMMAND_READ_VERIFY_SECTORS_EXT         0x42
#define HDC_COMMAND_ZERO_EXT                        0x44
#define HDC_COMMAND_WRITE_UNCORRECTABLE_EXT         0x45
#define HDC_COMMAND_READ_LOG_DMA_EXT                0x47
#define HDC_COMMAND_ZAC_MAMAGEMENT_IN               0x4A
#define HDC_COMMAND_CONFIGURE_STREAM                0x51
#define HDC_COMMAND_WRITE_LOG_DMA_EXT               0x57
#define HDC_COMMAND_TRUSTED_NON_DATA                0x5B
#define HDC_COMMAND_TRUSTED_RECEIVE                 0x5C
#define HDC_COMMAND_TRUSTED_RECEIVE_DMA             0x5D
#define HDC_COMMAND_TRUSTED_SEND                    0x5E
#define HDC_COMMAND_TRUSTED_SEND_DMA                0x5F
#define HDC_COMMAND_READ_FPDMA_QUEUED               0x60
#define HDC_COMMAND_WRITE_FPDMA_QUEUED              0x61
#define HDC_COMMAND_NCQ_NON_DATA                    0x63
#define HDC_COMMAND_SEND_FPDMA_QUEUED               0x64
#define HDC_COMMAND_RECEIVE_FPDMA_QUEUED            0x65
#define HDC_COMMAND_SET_DATE_AND_TIME_EXT           0x77
#define HDC_COMMAND_ACCESSIBLE_MAX_ADDR_CONFIG      0x78
#define HDC_COMMAND_REMOVE_ELEMENT_AND_TRUNCATE     0x7C
#define HDC_COMMAND_RESTORE_ELEMENTS_AND_REBUILD    0x7D
#define HDC_COMMAND_REMOVE_ELEMT_AND_MODIFY_ZONES   0x7E
#define HDC_COMMAND_CFA_TRANSLATE_SECTOR            0x87
#define HDC_COMMAND_EXECUTE_SEVICE_DIAGNOSTIC       0x90
#define HDC_COMMAND_DOWNLOAD_MICROCODE              0x92
#define HDC_COMMAND_DOWNLOAD_MICROCODE_DMA          0x93
#define HDC_COMMAND_MUTATE_EXT                      0x96
#define HDC_COMMAND_ZAC_MANAGEMENT_OUT              0x9F
#define HDC_COMMAND_SMART                           0xB0
#define HDC_COMMAND_SET_SECTOR_CONFIG_EXT           0xB2
#define HDC_COMMAND_SANITIZE_DEVICE                 0xB4
#define HDC_COMMAND_READ_DMA                        0xC8
#define HDC_COMMAND_WRITE_DMA                       0xCA
#define HDC_COMMAND_WRITE_MULTIPLE_NO_ERASE         0xCD
#define HDC_COMMAND_STANDBY_IMMEDIATE               0xE0
#define HDC_COMMAND_IDLE_IMMEDIATE                  0xE1
#define HDC_COMMAND_STANDBY                         0xE2
#define HDC_COMMAND_IDLE                            0xE3
#define HDC_COMMAND_READ_BUFFER                     0xE4
#define HDC_COMMAND_CHECK_POWER_MODE                0xE5
#define HDC_COMMAND_SLEEP                           0xE6
#define HDC_COMMAND_FLUSH_CACHE                     0xE7
#define HDC_COMMAND_WRITE_BUFFER                    0xE8
#define HDC_COMMAND_READ_BUFFER_DMA                 0xE9
#define HDC_COMMAND_FLUSH_CACHE_EXT                 0xEA
#define HDC_COMMAND_WRITE_BUFFER_DMA                0xEB
#define HDC_COMMAND_IDENTIFY_DEVICE                 0xEC
#define HDC_COMMAND_SET_FEATURES                    0xEF
#define HDC_COMMAND_SET_PASSWORD                    0xF1
#define HDC_COMMAND_UNLOCK                          0xF2
#define HDC_COMMAND_ERASE_PREPARE                   0xF3
#define HDC_COMMAND_ERASE_UNIT                      0xF4
#define HDC_COMMAND_ERASE_LOCK                      0xF5
#define HDC_COMMAND_DISABLE_PASSWORD                0xF6

#endif // __PORTS_HDC_H
