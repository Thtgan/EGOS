#if !defined(__INDENTIFY_DEVICE_H)
#define __INDENTIFY_DEVICE_H

#include<kit/types.h>

typedef struct {
    struct {
        Uint8 reserved                  : 1;
        Uint8 retired1                  : 1;
        Uint8 responseIncomplete        : 1;
        Uint8 retired2                  : 3;
        Uint8 obsolete                  : 1;
        Uint8 removableDevice           : 1;
        Uint8 retired3                  : 7;
        Uint8 isNotATA                  : 1;
    } generalConfiguration;                             //Word 0
    Uint16 obsolete1;                                   //Word 1
    Uint16 specificConfiguration;                       //Word 2
    Uint16 obsolete2;                                   //Word 3
    Uint16 retried1[2];                                 //Word 4-5
    Uint16 obsolete3;                                   //Word 6
    Uint16 reserved1[2];                                //Word 7-8
    Uint16 retired2;                                    //Word 9
    char serialNumber[20];                              //Word 10-19
    Uint16 retired3[2];                                 //Word 20-21
    Uint16 obsolete4;                                   //Word 22
    char firmwareRevision[8];                           //Word 23-26
    char modelNumber[40];                               //Word 27-46
    Uint8 maxSectorPreMultipleTransfer;
    Uint8 reserved2;                                    //Word 47 (0x80 fixed)
    Uint16 reserved3;                                   //Word 48
    struct {
        Uint8 retired;
        Uint8 DMAsupported              : 1;
        Uint8 LBAsupported              : 1;
        Uint8 iordyDisabled             : 1;
        Uint8 iordySupported            : 1;            //May be supported if 0
        Uint8 reserved1                 : 1;
        Uint8 standbyTimerSupported     : 1;
        Uint8 reserved2                 : 2;
        Uint16 reserved3;
    } capabilities;                                     //Word 49-50
    Uint16 obsolete5[2];                                //Word 51-52
    struct {
        Uint16 obsolete                 : 1;
        Uint16 field64_70Valid          : 1;
        Uint16 field88Valid             : 1;
        Uint16 reserved                 : 13;
    } fieldValid;                                       //Word 53
    Uint16 obsolete6[5];                                //Word 54-58
    Uint8 sectorTransferNumPerIntOnMultiCmd;
    Uint8 multiSectorSettingValid       : 1;
    Uint8 reserved4                     : 7;            //Word 59
    Uint32 addressableSectorNum;                        //Word 60-61
    Uint16 obsolete7;                                   //Word 62
    struct {
        Uint8 multiwordMode0Supported       : 1;
        Uint8 multiwordMode1BelowSupported  : 1;
        Uint8 multiwordMode2BelowSupported  : 1;
        Uint8 reserved1                     : 5;
        Uint8 multiwordMode0Selected        : 1;
        Uint8 multiwordMode1Selected        : 1;
        Uint8 multiwordMode2Selected        : 1;
        Uint8 reserved2                     : 5;
    } DMAmode;                                          //Word 63
    Uint8 advancedPIOmodeSuppported;
    Uint8 reserved5;                                    //Word 64
    Uint16 minDMAwordTransferCycleTime;                 //Word 65 (in nanoseconds)
    Uint16 recommendedDMAwordTransferCycleTime;         //Word 66 (in nanoseconds)
    Uint16 minPIOtransferCycleTime;                     //Word 67
    Uint16 minPIOtransferCycleTimeIORDY;                //Word 68
    Uint16 reserved6[6];                                //Word 69-74
    struct {
        Uint16 maxQueueDepth            : 5;
        Uint16 reserved                 : 11;
    } queueDepth;                                       //Word 75
    Uint16 reserved7[4];                                //Word 76-79
    struct {
        Uint16 reserved1                : 1;
        Uint16 obsolete                 : 3;
        Uint16 ATA_ATAPI4Supported      : 1;
        Uint16 ATA_ATAPI5Supported      : 1;
        Uint16 ATA_ATAPI6Supported      : 1;
        Uint16 ATA_ATAPI7Supported      : 1;
        Uint16 reserved2                : 8;
    } majorVersion;                                     //Word 80
    Uint16 minorVision;                                 //Word 81
    struct {
        Uint32 SMARTsupported                               : 1;
        Uint32 securityModeSupported                        : 1;
        Uint32 removableMediaSupported                      : 1;
        Uint32 powerManagementSupported                     : 1;
        Uint32 reserved1                                    : 1;
        Uint32 writeCacheSupported                          : 1;
        Uint32 lookAheadSupported                           : 1;
        Uint32 releaseInterruptSupported                    : 1;
        Uint32 serviceInterruptSupported                    : 1;
        Uint32 deviceResetCmdSupported                      : 1;
        Uint32 hostProtectedAreaSupported                   : 1;
        Uint32 obsolete1                                    : 1;
        Uint32 writeBufferCmdSupported                      : 1;
        Uint32 readBufferCmdSupported                       : 1;
        Uint32 nopCmdSupported                              : 1;
        Uint32 obsolete2                                    : 1;
        Uint32 downloadMicrocodeCmdSupported                : 1;
        Uint32 rwDMAqueuedSupported                         : 1;
        Uint32 CFAsupported                                 : 1;
        Uint32 advancedPowerManagementSupported             : 1;
        Uint32 removableMediaStatusNotificationSupported    : 1;
        Uint32 powerupInStandbySupported                    : 1;
        Uint32 setFeatureRequiredAfterPowerup               : 1;
        Uint32 reserved2                                    : 1;
        Uint32 setMaxSecurityExtensionSupported             : 1;
        Uint32 automaticAcousticManagementSupported         : 1;
        Uint32 lba48Supported                               : 1;
        Uint32 deviceConfigurationOverlaySupported          : 1;
        Uint32 mandatoryFlushCacheCmdSupported              : 1;
        Uint32 flushCacheExtCmdSupported                    : 1;
        Uint32 reserved3                                    : 2;
    } commandSetSupport;                                //Word 82-83
    struct {
        Uint16 SMARTerrorLoggingSupported               : 1;
        Uint16 SMARTselfTestSupported                   : 1;
        Uint16 mediaSerialNumberSupported               : 1;
        Uint16 mediaCardPassThroughCmdSupported         : 1;
        Uint16 streamingSupported                       : 1;
        Uint16 generalPurposeLoggingSupported           : 1;
        Uint16 writeDMA_MultipleFUAextCmdSupported      : 1;
        Uint16 writeDMAqueuedFUAextCmdSupported         : 1;
        Uint16 worldWideNameSupported                   : 1;
        Uint16 urgBitForReadStream_DMA_ExtSupported     : 1;
        Uint16 urgBitForWriteStream_DMA_ExtSupported    : 1;
        Uint16 reserved1                                : 2;
        Uint16 idleImmediate_unloadSupported            : 1;
        Uint16 reserved2                                : 2;
    } commandSet_featureSupportedExtension;             //Word 84
    struct {
        Uint32 SMARTenabled                             : 1;
        Uint32 securityModeEnabled                      : 1;
        Uint32 removableMediaEnabled                    : 1;
        Uint32 powerManagementEnabled                   : 1;
        Uint32 reserved1                                : 1;
        Uint32 writeCacheEnabled                        : 1;
        Uint32 lookAheadEnabled                         : 1;
        Uint32 releaseInterruptEnabled                  : 1;
        Uint32 serviceInterruptEnabled                  : 1;
        Uint32 deviceResetCmdEnabled                    : 1;
        Uint32 hostProtectedAreaEnabled                 : 1;
        Uint32 obsolete1                                : 1;
        Uint32 writeBufferCmdSupported                  : 1;
        Uint32 readBufferCmdSupported                   : 1;
        Uint32 nopCmdSupported                          : 1;
        Uint32 obsolete2                                : 1;
        Uint32 downloadMicrocodeCmdEnabled              : 1;
        Uint32 rwDMAqueuedEnabled                       : 1;
        Uint32 CFAenabled                               : 1;
        Uint32 advancedPowerManagementEnabled           : 1;
        Uint32 removableMediaStatusNotificationEnabled  : 1;
        Uint32 powerupInStandbyEnabled                  : 1;
        Uint32 setFeatureRequiredAfterPowerup           : 1;
        Uint32 reserved2                                : 1;
        Uint32 setMaxSecurityExtensionEnabled           : 1;
        Uint32 automaticAcousticManagementEnabled       : 1;
        Uint32 lba48Enabled                             : 1;
        Uint32 deviceConfigurationOverlayEnabled        : 1;
        Uint32 mandatoryFlushCacheCmdEnabled            : 1;
        Uint32 flushCacheExtCmdEnabled                  : 1;
        Uint32 reserved3                                : 2;
    } commandSet_featureEnabled;                        //Word 85-86
    struct {
        Uint16 SMARTerrorLoggingSupported               : 1;
        Uint16 SMARTselfTestSupported                   : 1;
        Uint16 mediaSerialNumberSupported               : 1;
        Uint16 mediaCardPassThroughCmdSupported         : 1;
        Uint16 configureStreamCmdExecuted               : 1;
        Uint16 generalPurposeLoggingSupported           : 1;
        Uint16 writeDMA_MultipleFUAextCmdSupported      : 1;
        Uint16 writeDMAqueuedFUAextCmdSupported         : 1;
        Uint16 worldWideNameSupported                   : 1;
        Uint16 urgBitForReadStream_DMA_ExtSupported     : 1;
        Uint16 urgBitForWriteStream_DMA_ExtSupported    : 1;
        Uint16 reserved1                                : 2;
        Uint16 idleImmediate_unloadSupported            : 1;
        Uint16 reserved2                                : 2;
    } commandSet_featureSupportedDefault;               //Word 87
    struct {
        Uint16 mode0Supported           : 1;
        Uint16 mode1BelowSupported      : 1;
        Uint16 mode2BelowSupported      : 1;
        Uint16 mode3BelowSupported      : 1;
        Uint16 mode4BelowSupported      : 1;
        Uint16 mode5BelowSupported      : 1;
        Uint16 mode6BelowSupported      : 1;
        Uint16 reserved1                : 1;
        Uint16 mode0Selected            : 1;
        Uint16 mode1Selected            : 1;
        Uint16 mode2Selected            : 1;
        Uint16 mode3Selected            : 1;
        Uint16 mode4Selected            : 1;
        Uint16 mode5Selected            : 1;
        Uint16 mode6Selected            : 1;
        Uint16 reserved2                : 1;
    } ultraDMA;                                         //Word 88
    Uint16 securityEraseRequiredTime;                   //Word 89
    Uint16 enhancedSecurityEraseRequiredTime;           //Word 90
    Uint16 currentAdvancedPowerManagementVal;           //Word 91
    Uint16 masterPasswordRevisionCode;                  //Word 92
    struct {
        Uint16 reserved1                            : 1;
        Uint16 device0NumberDetermineMethod         : 2;
#define DEVICE_IDENTIFY_DATA_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_RESERVED   0b00
#define DEVICE_IDENTIFY_DATA_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_JUMPER     0b01
#define DEVICE_IDENTIFY_DATA_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_CSEL       0b10
#define DEVICE_IDENTIFY_DATA_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_UNKNOWN    0b11
        Uint16 device0DiagnosticsPassed             : 1;
        Uint16 device0PDIAGassertionDetected        : 1;
        Uint16 device0DASPassertionDetected         : 1;
        Uint16 device0RespondInDevice1Selected      : 1;
        Uint16 reserved2                            : 2;
        Uint16 device1NumberDetermineMethod         : 2;
        Uint16 device1PDIAGasserted                 : 1;
        Uint16 reserved3                            : 1;
        Uint16 CBLIDAbove                           : 1;
        Uint16 reserved4                            : 2;
    } hardwareResetResult;                              //Word 93
    Uint8 currentAutomaticAcousticManagementVal;
    Uint8 recommendedAutomaticAcousticManagementVal;    //Word 94
    Uint16 streamMinRequestSize;                        //Word 95
    Uint16 DMAstreamingTransferTime;                    //Word 96
    Uint16 DMA_PIOstreamingAccessLatency;               //Word 97
    Uint32 streamingPerformanceGranularity;             //Word 98-99
    Uint64 maxUserLBAfor48bitAddress;                   //Word 100-103
    Uint16 PIOstreamingTransferTime;                    //Word 104
    Uint16 reserved8;                                   //Word 105
    struct {
        Uint16 LogicalSectorPerPhysicalSector               : 4;    //2^x
        Uint16 reserved1                                    : 8;
        Uint16 logicalSectorLongerThan256Words              : 1;
        Uint16 hasMultipleLogicalSectorPerPhysicalSector    : 1;
        Uint16 reserved2                                    : 2;
    } physical_logicalSectorSize;                       //Word 106
    Uint16 interSeekDelay;                              //Word 107 (in microseconds)
    Uint16 IEEE_OUI12_23    : 12;
    Uint16 NAA              : 4;                        //Word 108
    Uint16 uniqueID32_35    : 4;
    Uint16 uniqueID0_11     : 12;                       //Word 109
    Uint16 uniqueID16_31;                               //Word 110
    Uint16 uniqueID0_15;                                //Word 111
    Uint16 reserved9[5];                                //Word 112-116
    Uint32 wordsPerLogicalSector;                       //Word 117-118
    Uint16 reserved10[8];                               //Word 119-126
    Uint16 removableMediaStatusNotificationFeatureSupported : 1;
    Uint16 reserved11                           : 15;   //Word 127
    struct {
        Uint16 supported                        : 1;
        Uint16 enabled                          : 1;
        Uint16 locked                           : 1;
        Uint16 frozen                           : 1;
        Uint16 securityCountExpired             : 1;
        Uint16 enhancedSecurityEraseSupported   : 1;
        Uint16 reserved1                        : 2;
        Uint16 isMaxSecurityLevel               : 1;
        Uint16 reserved2                        : 7;
    } securityStatus;                                   //Word 128
    Uint16 vendorSpecific[31];                          //Word 129-159
    struct {
        Uint16 maxCurrent               : 12;           //in mA
        Uint16 disabled                 : 1;
        Uint16 requiredForCommands      : 1;
        Uint16 reserved                 : 1;
        Uint16 word160Supported         : 1;
    } CFApowerMode1;                                    //Word 160
    Uint16 reserved12[15];                              //Word 161-175
    char currentMediaSerialNumber[60];                  //Word 176-205
    Uint16 reserved13[49];                              //Word 206-254
    struct {
        Uint8 signature;
        Uint8 checksum;
    } integrity;                                        //Word 255
} __attribute__((packed)) DeviceIdentifyData;

typedef struct {
    struct {
        Uint8 commandPacketLength   : 2;
#define PACKET_DEVICE_IDENTIFY_DATA_GENERAL_CONFIGURATION_COMMAND_PACKET_LENGTH_12_BYTE 0b00
#define PACKET_DEVICE_IDENTIFY_DATA_GENERAL_CONFIGURATION_COMMAND_PACKET_LENGTH_16_BYTE 0b01
        Uint8 responseIncomplete    : 1;
        Uint8 reserved1             : 2;
        Uint8 deviceSetDRQtime      : 2;
#define PACKET_DEVICE_IDENTIFY_DATA_GENERAL_CONFIGURATION_DEVICE_SET_DRQ_TIME_3MS       0b00
#define PACKET_DEVICE_IDENTIFY_DATA_GENERAL_CONFIGURATION_DEVICE_SET_DRQ_TIME_50US      0b10
        Uint8 removableDevice       : 1;
        Uint8 commandPacketSet      : 5;
        Uint8 reserved2             : 1;
        Uint8 isATAPI               : 2;
#define PACKET_DEVICE_IDENTIFY_DATA_GENERAL_CONFIGURATION_IS_ATAPI                      0b10
    } generalConfiguration;                             //Word 0
    Uint16 reserved1;                                   //Word 1
    Uint16 uniqueConfiguration;                         //Word 2
    Uint16 reserved2[7];                                //Word 3-9
    char serialNumber[20];                              //Word 10-19
    Uint16 reserved3[3];                                //Word 20-22
    char firmwareRevision[8];                           //Word 23-26
    char modelNumber[40];                               //Word 27-46
    Uint16 reserved4[2];                                //Word 47-48
    struct {
        Uint8 vendorSpecific;
        Uint8 DMAsupported              : 1;
        Uint8 reserved1                 : 1;
        Uint8 iordyMayBeDisabled        : 1;
        Uint8 iordySupported            : 1;
        Uint8 obsolete                  : 1;
        Uint8 overlapOperationSupported : 1;
        Uint8 commandQueuingSupported   : 1;
        Uint8 interleavedDMAsupported   : 1;
        Uint16 reserved2;
    } capabilities;                                     //Word 49-50
    Uint16 obsolete2[2];                                //Word 51-52
    struct {
        Uint16 obsolete                 : 1;
        Uint16 field64_70Valid          : 1;
        Uint16 field88Valid             : 1;
        Uint16 reserved                 : 13;
    } fieldValid;                                       //Word 53;
    Uint16 reserved5[8];                                //Word 54-61
    struct {
        Uint16 ultraMode0Supported          : 1;
        Uint16 ultraMode1BelowSupported     : 1;
        Uint16 ultraMode2BelowSupported     : 1;
        Uint16 ultraMode3BelowSupported     : 1;
        Uint16 ultraMode4BelowSupported     : 1;
        Uint16 ultraMode5BelowSupported     : 1;
        Uint16 ultraMode6BelowSupported     : 1;
        Uint16 multiwordMode0supported      : 1;
        Uint16 multiwordMode1supported      : 1;
        Uint16 multiwordMode2supported      : 1;
        Uint16 DMAsupported                 : 1;
        Uint16 reserved1                    : 4;
        Uint16 DMADIRbitRequired            : 1;
        Uint16 multiwordMode0Supported      : 1;
        Uint16 multiwordMode1BelowSupported : 1;
        Uint16 multiwordMode2BelowSupported : 1;
        Uint16 reserved2                    : 5;
        Uint16 multiwordMode0Selected       : 1;
        Uint16 multiwordMode1Selected       : 1;
        Uint16 multiwordMode2Selected       : 1;
        Uint16 reserved3                    : 5;
    } DMAmode;                                          //Word 62-63
    Uint8 advancedPIOmodeSuppported;
    Uint8 reserved6;                                    //Word 64
    Uint16 minDMAwordTransferCycleTime;                 //Word 65 (in nanoseconds)
    Uint16 recommendedDMAwordTransferCycleTime;         //Word 66 (in nanoseconds)
    Uint16 minPIOtransferCycleTime;                     //Word 67
    Uint16 minPIOtransferCycleTimeIORDY;                //Word 68
    Uint16 reserved7[2];                                //Word 69-70
    Uint16 typicalTimeOfPACKETcmdToBusRelease;          //Word 71 (in nanoseconds)
    Uint16 typicalTimeOfSERVICEcmdToBSYcleared;         //Word 72 (in nanoseconds)
    Uint16 reserved8[2];                                //Word 73-74
    struct {
        Uint16 maxQueueDepth            : 5;
        Uint16 reserved                 : 11;
    } queueDepth;                                       //Word 75
    Uint16 reserved9[4];                                //Word 76-79
    struct {
        Uint16 reserved1                : 1;
        Uint16 obsolete                 : 3;
        Uint16 ATA_ATAPI4Supported      : 1;
        Uint16 ATA_ATAPI5Supported      : 1;
        Uint16 ATA_ATAPI6Supported      : 1;
        Uint16 ATA_ATAPI7Supported      : 1;
        Uint16 reserved2                : 8;
    } majorVersion;                                     //Word 80
    Uint16 minorVision;                                 //Word 81
    struct {
        Uint32 SMARTsupported                               : 1;
        Uint32 securityModeSupported                        : 1;
        Uint32 removableMediaSupported                      : 1;
        Uint32 powerManagementSupported                     : 1;
        Uint32 reserved1                                    : 1;
        Uint32 writeCacheSupported                          : 1;
        Uint32 lookAheadSupported                           : 1;
        Uint32 releaseInterruptSupported                    : 1;
        Uint32 serviceInterruptSupported                    : 1;
        Uint32 deviceResetCmdSupported                      : 1;
        Uint32 hostProtectedAreaSupported                   : 1;
        Uint32 obsolete1                                    : 1;
        Uint32 writeBufferCmdSupported                      : 1;
        Uint32 readBufferCmdSupported                       : 1;
        Uint32 nopCmdSupported                              : 1;
        Uint32 obsolete2                                    : 1;
        Uint32 downloadMicrocodeCmdSupported                : 1;
        Uint32 reserved3                                    : 3;
        Uint32 removableMediaStatusNotificationSupported    : 1;
        Uint32 powerupInStandbySupported                    : 1;
        Uint32 setFeatureRequiredAfterPowerup               : 1;
        Uint32 reserved4                                    : 1;
        Uint32 setMaxSecurityExtensionSupported             : 1;
        Uint32 automaticAcousticManagementSupported         : 1;
        Uint32 reserved5                                    : 1;
        Uint32 deviceConfigurationOverlaySupported          : 1;
        Uint32 flushCacheExtCmdSupported                    : 1;
        Uint32 reserved6                                    : 3;
    } commandSetSupport;                                //Word 82-83
    Uint16 reserved10;                                  //Word 84
    struct {
        Uint32 SMARTenabled                             : 1;
        Uint32 securityModeEnabled                      : 1;
        Uint32 removableMediaEnabled                    : 1;
        Uint32 powerManagementEnabled                   : 1;
        Uint32 reserved1                                : 1;
        Uint32 writeCacheEnabled                        : 1;
        Uint32 lookAheadEnabled                         : 1;
        Uint32 releaseInterruptEnabled                  : 1;
        Uint32 serviceInterruptEnabled                  : 1;
        Uint32 deviceResetCmdEnabled                    : 1;
        Uint32 hostProtectedAreaEnabled                 : 1;
        Uint32 obsolete1                                : 1;
        Uint32 writeBufferCmdSupported                  : 1;
        Uint32 readBufferCmdSupported                   : 1;
        Uint32 nopCmdSupported                          : 1;
        Uint32 obsolete2                                : 1;
        Uint32 downloadMicrocodeCmdEnabled              : 1;
        Uint32 reserved2                                : 3;
        Uint32 removableMediaStatusNotificationEnabled  : 1;
        Uint32 powerupInStandbyEnabled                  : 1;
        Uint32 setFeatureRequiredAfterPowerup           : 1;
        Uint32 reserved3                                : 1;
        Uint32 setMaxSecurityExtensionEnabled           : 1;
        Uint32 automaticAcousticManagementEnabled       : 1;
        Uint32 reserved4                                : 1;
        Uint32 deviceConfigurationOverlayEnabled        : 1;
        Uint32 mandatoryFlushCacheCmdEnabled            : 1;
        Uint32 reserved5                                : 3;
    } commandSet_featureEnabled;                        //Word 85-86
    Uint16 reserved11;                                  //Word 87
    struct {
        Uint16 mode0Supported           : 1;
        Uint16 mode1BelowSupported      : 1;
        Uint16 mode2BelowSupported      : 1;
        Uint16 mode3BelowSupported      : 1;
        Uint16 mode4BelowSupported      : 1;
        Uint16 mode5BelowSupported      : 1;
        Uint16 mode6BelowSupported      : 1;
        Uint16 reserved1                : 1;
        Uint16 mode0Selected            : 1;
        Uint16 mode1Selected            : 1;
        Uint16 mode2Selected            : 1;
        Uint16 mode3Selected            : 1;
        Uint16 mode4Selected            : 1;
        Uint16 mode5Selected            : 1;
        Uint16 mode6Selected            : 1;
        Uint16 reserved2                : 1;
    } ultraDMA;                                         //Word 88
    Uint16 reserved12[4];                               //Word 89-92
    struct {
        Uint16 reserved1                            : 1;
        Uint16 device0NumberDetermineMethod         : 2;
#define PACKET_DEVICE_IDENTIFY_DATA_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_RESERVED   0b00
#define PACKET_DEVICE_IDENTIFY_DATA_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_JUMPER     0b01
#define PACKET_DEVICE_IDENTIFY_DATA_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_CSEL       0b10
#define PACKET_DEVICE_IDENTIFY_DATA_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_UNKNOWN    0b11
        Uint16 device0DiagnosticsPassed             : 1;
        Uint16 device0PDIAGassertionDetected        : 1;
        Uint16 device0DASPassertionDetected         : 1;
        Uint16 device0RespondInDevice1Selected      : 1;
        Uint16 reserved2                            : 2;
        Uint16 device1NumberDetermineMethod         : 2;
        Uint16 device1PDIAGasserted                 : 1;
        Uint16 reserved3                            : 1;
        Uint16 CBLIDAbove                           : 1;
        Uint16 reserved4                            : 2;
    } hardwareResetResult;                              //Word 93
    Uint8 currentAutomaticAcousticManagementVal;
    Uint8 recommendedAutomaticAcousticManagementVal;    //Word 94
    Uint16 reserved13[30];                              //Word 95-124
    Uint16 ATAPIbyteCount0Behavior;                     //Word 125
    Uint16 obsolete3;                                   //Word 126
    Uint16 removableMediaStatusNotificationFeatureSupported : 1;
    Uint16 reserved14 : 15;                             //Word 127
    struct {
        Uint16 supported                            : 1;
        Uint16 enabled                              : 1;
        Uint16 locked                               : 1;
        Uint16 frozen                               : 1;
        Uint16 securityCountExpired                 : 1;
        Uint16 enhancedSecurityEraseSupported       : 1;
        Uint16 reserved1                            : 2;
        Uint16 isMaxSecurityLevel                   : 1;
        Uint16 reserved2                            : 7;
    } securityStatus;                                   //Word 128
    Uint16 vendorSpecific[31];                          //Word 129-159
    Uint16 reserved15[95];                              //Word 160-254
    struct {
        Uint8 signature;
        Uint8 checksum;
    } integrity;                                        //Word 255
} __attribute__((packed)) PacketDeviceIdentifyData;

#endif // __INDENTIFY_DEVICE_H
