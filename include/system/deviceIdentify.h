#if !defined(__DEVICE_INDETIFY_H)
#define __DEVICE_INDETIFY_H

#include<kit/types.h>

typedef struct {
    //Word 0:
    struct {
        Uint8 reserved              : 1;
        Uint8 retired1              : 1;
        Uint8 responseIncomplete    : 1;
        Uint8 retired2              : 3;
        Uint8 fixedDevice           : 1;
        Uint8 removableDevice       : 1;
        Uint8 retired3              : 7;
        Uint8 isNotATA              : 1;
    } generalConfiguration;
    Uint16 cylinderNum;
    Uint16 specificConfiguration;
    Uint16 headNum;
    Uint16 retried1[2];
    Uint16 sectorNumPerTrack;
    Uint16 reserved1[2];
    Uint16 retired2;
    //Word 10:
    char serialNumber[20];
    Uint16 retired3[2];
    Uint16 obsolete1;
    char firmwareRevision[8];
    //Word 27:
    char modelNumber[40];
    Uint8 maximumSectorPreTransfer;
    Uint8 reserved2;  //0x80 fixed
    Uint16 reserved3;
    //Word 49:
    struct {
        Uint8 retired;
        Uint8 reserved1                 : 2;    //Should be 0b11
        Uint8 iordyDisabled             : 1;
        Uint8 iordySupported            : 1;
        Uint8 reserved2                 : 1;
        Uint8 standbyTimerSupported     : 1;
        Uint8 reserved3                 : 2;
        Uint16 reserved4;
    } capabilities;
    Uint16 obsolete2[2];
    Uint8 field54_58Valid   : 1;
    Uint8 field64_70Valid   : 1;
    Uint8 field80Valid      : 1;
    Uint8 reserved4         : 5;
    Uint8 reserved5;
    Uint16 currentCylinderNum;
    Uint16 currentHeadNum;
    Uint16 currentSectorNumPreTrack;
    Uint32 currentSectorCapacity;
    Uint8 currentMultiSectorTransferSetting;
    Uint8 multiSectorSettingValid   : 1;
    Uint8 reserved7 : 7;
    //Word 60:
    Uint32 addressableSectorNum;
    Uint16 obsolete3;
    struct {
        Uint8 mode0Supported        : 1;
        Uint8 mode1BelowSupported   : 1;
        Uint8 mode2BelowSupported   : 1;
        Uint8 reserved1             : 5;
        Uint8 mode0Selected         : 1;
        Uint8 mode1Selected         : 1;
        Uint8 mode2Selected         : 1;
        Uint8 reserved2             : 5;
    } dmaMultiword;
    Uint8 advancedPIOmodeSuppported;
    Uint8 reserved8;
    Uint16 minDMAwordTransferCycleTime; //in nanoseconds
    Uint16 recommendedDMAwordTransferCycleTime; //in nanoseconds
    Uint16 minPIOtransferCycleTime;
    Uint16 minPIOtransferCycleTimeIORDY;
    //Word 69:
    Uint16 reserved9[6];
    Uint16 queueDepth   : 5;
    Uint16 reserved     : 11;
    Uint16 reserved10[4];
    //Word 80:
    struct {
        Uint16 reserved1                : 1;
        Uint16 obsolete                 : 1;
        Uint16 ATA2Supported            : 1;
        Uint16 ATA3Supported            : 1;
        Uint16 ATA_ATAPI4Supported      : 1;
        Uint16 ATA_ATAPI5Supported      : 1;
        Uint16 reserved                 : 10;
    } majorVersion;
    Uint16 minorVision;
    struct {
        Uint16 smartFeatureSupport                                  : 1;
        Uint16 securityModeFeatureSupported                         : 1;
        Uint16 removableMediaFeatureSupported                       : 1;
        Uint16 powerManagementFeatureSupported                      : 1;
        Uint16 reserved1                                            : 1;
        Uint16 writeCacheSupported                                  : 1;
        Uint16 lookAheadSupported                                   : 1;
        Uint16 releaseInterruptSupported                            : 1;
        Uint16 serviceInterruptSupported                            : 1;
        Uint16 deviceResetSupported                                 : 1;
        Uint16 hostProtectedAreaFeatureSupported                    : 1;
        Uint16 obsolete1                                            : 1;
        Uint16 writeBufferSupported                                 : 1;
        Uint16 readBufferSupported                                  : 1;
        Uint16 nopSupported                                         : 1;
        Uint16 obsolete2                                            : 1;
        Uint16 downloadMicrocodeSupported                           : 1;
        Uint16 rwDMAqueuedSupported                                 : 1;
        Uint16 cfaFeaturesSupported                                 : 1;
        Uint16 advancedPowerManagementFeatureSupported              : 1;
        Uint16 removableMediaStatusNotificationFeatureSupported     : 1;
        Uint16 powerupInStandbyFeatureSupported                     : 1;
        Uint16 setFeatureRequiredAfterPowerup                       : 1;
        Uint16 reserved2                                            : 1;
        Uint16 setMaxSecurityExtensionSupported                     : 1;
        Uint16 reserved3                                            : 7;
    } commandSetSupport;
    struct {
        Uint16 reserved;
    } commandSet_featureSupportedExtension;
    struct {
        Uint16 smartFeatureSupport                              : 1;
        Uint16 securityModeFeatureEnabled                       : 1;
        Uint16 removableMediaFeatureEnabled                     : 1;
        Uint16 powerManagementFeatureEnabled                    : 1;
        Uint16 reserved1                                        : 1;
        Uint16 writeCacheEnabled                                : 1;
        Uint16 lookAheadEnabled                                 : 1;
        Uint16 releaseInterruptEnabled                          : 1;
        Uint16 serviceInterruptEnabled                          : 1;
        Uint16 deviceResetEnabled                               : 1;
        Uint16 hostProtectedAreaFeatureEnabled                  : 1;
        Uint16 obsolete1                                        : 1;
        Uint16 writeBufferEnabled                               : 1;
        Uint16 readBufferEnabled                                : 1;
        Uint16 nopEnabled                                       : 1;
        Uint16 obsolete2                                        : 1;
        Uint16 downloadMicrocodeSupported                       : 1;
        Uint16 rwDMAqueuedSupported                             : 1;
        Uint16 cfaFeaturesEnabled                               : 1;
        Uint16 advancedPowerManagementFeatureEnabled            : 1;
        Uint16 removableMediaStatusNotificationFeatureEnabled   : 1;
        Uint16 powerupInStandbyFeatureEnabled                   : 1;
        Uint16 setFeatureRequiredAfterPowerup                   : 1;
        Uint16 reserved2                                        : 1;
        Uint16 setMaxSecurityExtensionEnabled                   : 1;
        Uint16 reserved3                                        : 7;
    } commandSet_featureEnabled;
    struct {
        Uint16 reserved;
    } commandSet_featureSupportedDefault;
    struct {
        Uint16 mode0Supported           : 1;
        Uint16 mode1BelowSupported      : 1;
        Uint16 mode2BelowSupported      : 1;
        Uint16 mode3BelowSupported      : 1;
        Uint16 mode4BelowSupported      : 1;
        Uint16 reserved1                : 3;
        Uint16 mode0Selected            : 1;
        Uint16 mode1Selected            : 1;
        Uint16 mode2Selected            : 1;
        Uint16 mode3Selected            : 1;
        Uint16 mode4Selected            : 1;
        Uint16 reserved2                : 3;
    } ultraDMA;
    Uint16 securityEraseRequiredTime;
    //Word 90:
    Uint16 enhancedSecurityEraseRequiredTime;
    Uint16 currentAdvancedPowerManagementVal;
    Uint16 masterPasswordID;
    struct {
        Uint16 reserved1                            : 1;
        Uint16 device0NumberDetermineMethod         : 2;
        Uint16 device0DiagnosticsPassed             : 1;
        Uint16 device0PDIAGassertionDetected        : 1;
        Uint16 device0DASPassertionDetected         : 1;
        Uint16 device0RespondInDevice1Selected      : 1;
        Uint16 reserved2                            : 2;
        Uint16 device1NumberDetermineMethod         : 2;
        Uint16 device1PDIAGasserted                 : 1;
        Uint16 reserved3                            : 1;
        Uint16 cblidAbove                           : 1;
        Uint16 reserved4                            : 2;
    } hardwareReserResult;
    Uint16 reserved11[33];
    Uint16 removableMediaStatusNotificationFeatureSupported : 1;
    Uint16 reserved12 : 15;
    struct {
        Uint16 supported                : 1;
        Uint16 enabled                  : 1;
        Uint16 locked                   : 1;
        Uint16 frozen                   : 1;
        Uint16 securityCountExpired     : 1;
        Uint16 reserved1                : 2;
        Uint16 maxSecurityLevel         : 1;
        Uint16 reserved2                : 1;
    } securityStatus;
    Uint16 vendorSpecific[31];
    struct {
        Uint16 maxCurrent               : 12; //in mA
        Uint16 disabled                 : 1;
        Uint16 requiredForCommands      : 1;
        Uint16 reserved                 : 1;
        Uint16 word160Supported         : 1;
    } cfaPowerMode1;
    Uint16 reserved13[94];
    Uint8 signature;
    Uint8 checksum;
} __attribute__((packed)) DeviceIdentifyData;

#define DEVICE_IDENTIFY_MINOR_VISION_NO_REPORT_1    0x0000
#define DEVICE_IDENTIFY_MINOR_VISION_NO_REPORT_2    0xFFFF

#define DEVICE_IDENTIFY_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_RESERVED   0b00
#define DEVICE_IDENTIFY_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_JUMPER     0b01
#define DEVICE_IDENTIFY_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_CSEL       0b10
#define DEVICE_IDENTIFY_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_UNKNOWN    0b11

#endif // __DEVICE_INDETIFY_H
