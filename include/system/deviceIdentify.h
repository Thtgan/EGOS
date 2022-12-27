#if !defined(__DEVICE_INDETIFY_H)
#define __DEVICE_INDETIFY_H

#include<kit/types.h>

typedef struct {
    //Word 0:
    struct {
        uint8_t reserved            : 1;
        uint8_t retired1            : 1;
        uint8_t responseIncomplete  : 1;
        uint8_t retired2            : 3;
        uint8_t fixedDevice         : 1;
        uint8_t removableDevice     : 1;
        uint8_t retired3            : 7;
        uint8_t isNotATA            : 1;
    } generalConfiguration;
    uint16_t cylinderNum;
    uint16_t specificConfiguration;
    uint16_t headNum;
    uint16_t retried1[2];
    uint16_t sectorNumPerTrack;
    uint16_t reserved1[2];
    uint16_t retired2;
    //Word 10:
    char serialNumber[20];
    uint16_t retired3[2];
    uint16_t obsolete1;
    char firmwareRevision[8];
    //Word 27:
    char modelNumber[40];
    uint8_t maximumSectorPreTransfer;
    uint8_t reserved2;  //0x80 fixed
    uint16_t reserved3;
    //Word 49:
    struct {
        uint8_t retired;
        uint8_t reserved1               : 2;    //Should be 0b11
        uint8_t iordyDisabled           : 1;
        uint8_t iordySupported          : 1;
        uint8_t reserved2               : 1;
        uint8_t standbyTimerSupported   : 1;
        uint8_t reserved3               : 2;
        uint16_t reserved4;
    } capabilities;
    uint16_t obsolete2[2];
    uint8_t field54_58Valid : 1;
    uint8_t field64_70Valid : 1;
    uint8_t field80Valid    : 1;
    uint8_t reserved4       : 5;
    uint8_t reserved5;
    uint16_t currentCylinderNum;
    uint16_t currentHeadNum;
    uint16_t currentSectorNumPreTrack;
    uint32_t currentSectorCapacity;
    uint8_t currentMultiSectorTransferSetting;
    uint8_t multiSectorSettingValid : 1;
    uint8_t reserved7 : 7;
    //Word 60:
    uint32_t addressableSectorNum;
    uint16_t obsolete3;
    struct {
        uint8_t mode0Supported      : 1;
        uint8_t mode1BelowSupported : 1;
        uint8_t mode2BelowSupported : 1;
        uint8_t reserved1           : 5;
        uint8_t mode0Selected       : 1;
        uint8_t mode1Selected       : 1;
        uint8_t mode2Selected       : 1;
        uint8_t reserved2           : 5;
    } dmaMultiword;
    uint8_t advancedPIOmodeSuppported;
    uint8_t reserved8;
    uint16_t minDMAwordTransferCycleTime; //in nanoseconds
    uint16_t recommendedDMAwordTransferCycleTime; //in nanoseconds
    uint16_t minPIOtransferCycleTime;
    uint16_t minPIOtransferCycleTimeIORDY;
    //Word 69:
    uint16_t reserved9[6];
    uint16_t queueDepth : 5;
    uint16_t reserved   : 11;
    uint16_t reserved10[4];
    //Word 80:
    struct {
        uint16_t reserved1              : 1;
        uint16_t obsolete               : 1;
        uint16_t ATA2Supported          : 1;
        uint16_t ATA3Supported          : 1;
        uint16_t ATA_ATAPI4Supported    : 1;
        uint16_t ATA_ATAPI5Supported    : 1;
        uint16_t reserved               : 10;
    } majorVersion;
    uint16_t minorVision;
    struct {
        uint16_t smartFeatureSupport                                : 1;
        uint16_t securityModeFeatureSupported                       : 1;
        uint16_t removableMediaFeatureSupported                     : 1;
        uint16_t powerManagementFeatureSupported                    : 1;
        uint16_t reserved1                                          : 1;
        uint16_t writeCacheSupported                                : 1;
        uint16_t lookAheadSupported                                 : 1;
        uint16_t releaseInterruptSupported                          : 1;
        uint16_t serviceInterruptSupported                          : 1;
        uint16_t deviceResetSupported                               : 1;
        uint16_t hostProtectedAreaFeatureSupported                  : 1;
        uint16_t obsolete1                                          : 1;
        uint16_t writeBufferSupported                               : 1;
        uint16_t readBufferSupported                                : 1;
        uint16_t nopSupported                                       : 1;
        uint16_t obsolete2                                          : 1;
        uint16_t downloadMicrocodeSupported                         : 1;
        uint16_t rwDMAqueuedSupported                               : 1;
        uint16_t cfaFeaturesSupported                               : 1;
        uint16_t advancedPowerManagementFeatureSupported            : 1;
        uint16_t removableMediaStatusNotificationFeatureSupported   : 1;
        uint16_t powerupInStandbyFeatureSupported                   : 1;
        uint16_t setFeatureRequiredAfterPowerup                     : 1;
        uint16_t reserved2                                          : 1;
        uint16_t setMaxSecurityExtensionSupported                   : 1;
        uint16_t reserved3                                          : 7;
    } commandSetSupport;
    struct {
        uint16_t reserved;
    } commandSet_featureSupportedExtension;
    struct {
        uint16_t smartFeatureSupport                            : 1;
        uint16_t securityModeFeatureEnabled                     : 1;
        uint16_t removableMediaFeatureEnabled                   : 1;
        uint16_t powerManagementFeatureEnabled                  : 1;
        uint16_t reserved1                                      : 1;
        uint16_t writeCacheEnabled                              : 1;
        uint16_t lookAheadEnabled                               : 1;
        uint16_t releaseInterruptEnabled                        : 1;
        uint16_t serviceInterruptEnabled                        : 1;
        uint16_t deviceResetEnabled                             : 1;
        uint16_t hostProtectedAreaFeatureEnabled                : 1;
        uint16_t obsolete1                                      : 1;
        uint16_t writeBufferEnabled                             : 1;
        uint16_t readBufferEnabled                              : 1;
        uint16_t nopEnabled                                     : 1;
        uint16_t obsolete2                                      : 1;
        uint16_t downloadMicrocodeSupported                     : 1;
        uint16_t rwDMAqueuedSupported                           : 1;
        uint16_t cfaFeaturesEnabled                             : 1;
        uint16_t advancedPowerManagementFeatureEnabled          : 1;
        uint16_t removableMediaStatusNotificationFeatureEnabled : 1;
        uint16_t powerupInStandbyFeatureEnabled                 : 1;
        uint16_t setFeatureRequiredAfterPowerup                 : 1;
        uint16_t reserved2                                      : 1;
        uint16_t setMaxSecurityExtensionEnabled                 : 1;
        uint16_t reserved3                                      : 7;
    } commandSet_featureEnabled;
    struct {
        uint16_t reserved;
    } commandSet_featureSupportedDefault;
    struct {
        uint16_t mode0Supported         : 1;
        uint16_t mode1BelowSupported    : 1;
        uint16_t mode2BelowSupported    : 1;
        uint16_t mode3BelowSupported    : 1;
        uint16_t mode4BelowSupported    : 1;
        uint16_t reserved1              : 3;
        uint16_t mode0Selected          : 1;
        uint16_t mode1Selected          : 1;
        uint16_t mode2Selected          : 1;
        uint16_t mode3Selected          : 1;
        uint16_t mode4Selected          : 1;
        uint16_t reserved2              : 3;
    } ultraDMA;
    uint16_t securityEraseRequiredTime;
    //Word 90:
    uint16_t enhancedSecurityEraseRequiredTime;
    uint16_t currentAdvancedPowerManagementVal;
    uint16_t masterPasswordID;
    struct {
        uint16_t reserved1                          : 1;
        uint16_t device0NumberDetermineMethod       : 2;
        uint16_t device0DiagnosticsPassed           : 1;
        uint16_t device0PDIAGassertionDetected      : 1;
        uint16_t device0DASPassertionDetected       : 1;
        uint16_t device0RespondInDevice1Selected    : 1;
        uint16_t reserved2                          : 2;
        uint16_t device1NumberDetermineMethod       : 2;
        uint16_t device1PDIAGasserted               : 1;
        uint16_t reserved3                          : 1;
        uint16_t cblidAbove                         : 1;
        uint16_t reserved4                          : 2;
    } hardwareReserResult;
    uint16_t reserved11[33];
    uint16_t removableMediaStatusNotificationFeatureSupported : 1;
    uint16_t reserved12 : 15;
    struct {
        uint16_t supported              : 1;
        uint16_t enabled                : 1;
        uint16_t locked                 : 1;
        uint16_t frozen                 : 1;
        uint16_t securityCountExpired   : 1;
        uint16_t reserved1              : 2;
        uint16_t maxSecurityLevel       : 1;
        uint16_t reserved2              : 1;
    } securityStatus;
    uint16_t vendorSpecific[31];
    struct {
        uint16_t maxCurrent             : 12; //in mA
        uint16_t disabled               : 1;
        uint16_t requiredForCommands    : 1;
        uint16_t reserved               : 1;
        uint16_t word160Supported       : 1;
    } cfaPowerMode1;
    uint16_t reserved13[94];
    uint8_t signature;
    uint8_t checksum;
} __attribute__((packed)) DeviceIdentifyData;

#define DEVICE_IDENTIFY_MINOR_VISION_NO_REPORT_1    0x0000
#define DEVICE_IDENTIFY_MINOR_VISION_NO_REPORT_2    0xFFFF

#define DEVICE_IDENTIFY_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_RESERVED   0b00
#define DEVICE_IDENTIFY_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_JUMPER     0b01
#define DEVICE_IDENTIFY_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_CSEL       0b10
#define DEVICE_IDENTIFY_HARDWARE_RESET_RESULT_DEVICE_NUMBER_DETERMINE_METHOD_UNKNOWN    0b11

#endif // __DEVICE_INDETIFY_H
