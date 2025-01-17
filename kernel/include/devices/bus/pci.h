#if !defined(__DEVICES_BUS_PCI_H)
#define __DEVICES_BUS_PCI_H

typedef struct PCIcommonHeader PCIcommonHeader;
typedef struct PCIHeaderType0 PCIHeaderType0;
typedef struct PCIHeaderType1 PCIHeaderType1;
typedef struct PCIHeaderType2 PCIHeaderType2;
typedef struct PCIdevice PCIdevice;

#include<debug.h>
#include<kit/bit.h>
#include<kit/macro.h>
#include<kit/types.h>
#include<kit/util.h>
#include<real/ports/pci.h>
#include<real/simpleAsmLines.h>
#include<result.h>

#define PCI_MAX_REGISTER_NUM    POWER_2(PCI_CONFIG_ADDR_REGISTER_NUMBER_LENGTH)
#define PCI_MAX_FUNCTION_NUM    POWER_2(PCI_CONFIG_ADDR_FUNCTION_NUMBER_LENGTH)
#define PCI_MAX_DEVICE_NUM      POWER_2(PCI_CONFIG_ADDR_DEVICE_NUMBER_LENGTH)
#define PCI_MAX_BUS_NUM         POWER_2(PCI_CONFIG_ADDR_BUS_NUMBER_LENGTH)

typedef struct PCIcommonHeader {
    Uint16 vendorID;
#define PCI_COMMON_HEADER_INVALID_VENDOR_ID 0xFFFF
    Uint16 deviceID;
    Uint16 command;
    Uint16 status;
    Uint8 revisionID;
    Uint8 progIF;       //Programming Interface Byte
    Uint8 subclass;     //Specifies the specific function the device performs
    Uint8 classCode;    //Specifies the type of function the device performs
    Uint8 cacheLineSize;
    Uint8 latencyTimer;
    Uint8 headerType;
#define PCI_COMMON_HEADER_HEADER_TYPE_GET_TYPE(HEADER_TYPE)         EXTRACT_VAL(HEADER_TYPE, 8, 0, 2)
#define PCI_COMMON_HEADER_HEADER_TYPE_TYPE_GENERAL_DEVICE           0x0
#define PCI_COMMON_HEADER_HEADER_TYPE_TYPE_PCI_TO_PCI_BRIDGE        0x1
#define PCI_COMMON_HEADER_HEADER_TYPE_TYPE_PCI_TO_CARDBUS_BRIDGE    0x2
#define PCI_COMMON_HEADER_HEADER_TYPE_FLAG_MULTIPLE_FUNCTION        FLAG8(7)
    Uint8 BIST;         //Status and allows control of a devices BIST (built-in self test)
} __attribute__((packed)) PCIcommonHeader;

DEBUG_ASSERT_COMPILE(sizeof(PCIcommonHeader) == 16);

//Normal device
typedef struct PCIHeaderType0 {
    PCIcommonHeader commonHeader;
    Uint32 bar0;
    Uint32 bar1;
    Uint32 bar2;
    Uint32 bar3;
    Uint32 bar4;
    Uint32 bar5;
    Uint32 cardbusCISptr;
    Uint16 subsystemVendorID;
    Uint16 subsystemID;
    Uint32 expansionROMbaseAddr;
    Uint8 capabilitiesPtr;
    Uint8 reserved1[3];
    Uint32 reserved2;
    Uint8 interruptLine;
    Uint8 interruptPin;
    Uint8 minGrant;
    Uint8 maxGrant;
} __attribute__((packed)) PCIHeaderType0;

DEBUG_ASSERT_COMPILE(sizeof(PCIHeaderType0) == 64);

//PCI-to-PCI bridge
typedef struct PCIHeaderType1 {
    PCIcommonHeader commonHeader;
    Uint32 bar0;
    Uint32 bar1;
    Uint8 primaryBusNumber;
    Uint8 secondaryBusNumber;
    Uint8 subordinateBusNumber;
    Uint8 secondaryLatencyTimer;
    Uint8 ioBase;
    Uint8 ioLimit;
    Uint16 secondaryStatus;
    Uint16 memoryBase;
    Uint16 memoryLimit;
    Uint16 prefetchableMemoryBase;
    Uint16 prefetchableMemoryLimit;
    Uint32 prefetchableBaseUpper32;
    Uint32 prefetchableLimitUpper32;
    Uint16 ioBaseUpper16;
    Uint16 ioLimitUpper16;
    Uint8 capabilitiesPtr;
    Uint8 reserved[3];
    Uint32 expansionROMbaseAddr;
    Uint8 interruptLine;
    Uint8 interruptPin;
    Uint16 bridgeControl;
} __attribute__((packed)) PCIHeaderType1;

DEBUG_ASSERT_COMPILE(sizeof(PCIHeaderType1) == 64);

//PCI-to-CardBus bridge
typedef struct PCIHeaderType2 {
    PCIcommonHeader commonHeader;
    Uint32 cardBusSocketBaseAddr;
    Uint8 capabilitiesListOffset;
    Uint8 reserved;
    Uint16 secondaryStatus;
    Uint8 pciBusNumber;
    Uint8 cardBusBusNumber;
    Uint8 subordinateBusNumber;
    Uint8 secondaryLatencyTimer;
    Uint32 memoryBaseAddr0;
    Uint32 memoryLimit0;
    Uint32 memoryBaseAddr1;
    Uint32 memoryLimit1;
    Uint32 ioBaseAddr0;
    Uint32 ioLimit0;
    Uint32 ioBaseAddr1;
    Uint32 ioLimit1;
    Uint8 interruptLine;
    Uint8 interruptPin;
    Uint16 bridgeControl;
    Uint16 subsystemDeviceID;
    Uint16 subsystemVendorID;
    Uint32 legacyModeBaseAddress;
} __attribute__((packed)) PCIHeaderType2;

DEBUG_ASSERT_COMPILE(sizeof(PCIHeaderType2) == 72);

#define PCI_HEADER_FIELD_BEGIN(__HEADER_TYPE, __FIELD)  offsetof(__HEADER_TYPE, __FIELD)
#define PCI_HEADER_FIELD_LENGTH(__HEADER_TYPE, __FIELD) SIZE_OF_STRUCT_MEMBER(__HEADER_TYPE, __FIELD)

//TODO: Fill subclass macros

#define PCI_COMMON_HEADER_CLASS_CODE_BUILT_BEFORE_FINIALIZED                0x00

#define PCI_COMMON_HEADER_CLASS_CODE_MASS_STORAGE_CONTROLLER                0x01

#define PCI_COMMON_HEADER_CLASS_CODE_NETWORK_CONTROLLER                     0x02

#define PCI_COMMON_HEADER_CLASS_CODE_DISPLAY_CONTROLLER                     0x03

#define PCI_COMMON_HEADER_CLASS_CODE_MULTIMEDIA                             0x04

#define PCI_COMMON_HEADER_CLASS_CODE_MEMORY_CONTROLLER                      0x05

#define PCI_COMMON_HEADER_CLASS_CODE_BRIDGE                                 0x06
#define PCI_COMMON_HEADER_SUB_CALSS_PCI2PCI_BRIDGE                          0x04

#define PCI_COMMON_HEADER_CLASS_CODE_SIMPLE_COMMUNICATION_COLNRTROLLER      0x07

#define PCI_COMMON_HEADER_CLASS_CODE_BASE_SYSTEM_PERIPHERALS                0x08

#define PCI_COMMON_HEADER_CLASS_CODE_INPUT                                  0x09

#define PCI_COMMON_HEADER_CLASS_CODE_DOCKING_STATIONS                       0x0A

#define PCI_COMMON_HEADER_CLASS_CODE_PROCESSORS                             0x0B

#define PCI_COMMON_HEADER_CLASS_CODE_SERIAL_BUS_CONTROLLER                  0x0C

#define PCI_COMMON_HEADER_CLASS_CODE_WIRELESS_CONTROLLER                    0x0D

#define PCI_COMMON_HEADER_CLASS_CODE_INTELLIGENT_IO_CONTROLLER              0x0E

#define PCI_COMMON_HEADER_CLASS_CODE_SATELLITE_COMMUNICATION_COLNRTROLLER   0x0F

#define PCI_COMMON_HEADER_CLASS_CODE_EN_DE_CRYPTION_COLNRTROLLER            0x10

#define PCI_COMMON_HEADER_CLASS_CODE_DATA_ACQ_AND_SIG_PROC_CONTROLLER       0x11

static inline Uint32 pci_read(Uint32 addr) {
    outl(PCI_CONFIG_ADDR, addr | PCI_CONFIG_ADDR_ENABLE);
    return inl(PCI_CONFIG_DATA);
}

static inline void pci_write(Uint32 addr, Uint32 value) {
    outl(PCI_CONFIG_ADDR, addr | PCI_CONFIG_ADDR_ENABLE);
    outl(PCI_CONFIG_DATA, value);
}

#define PCI_HEADER_FIELD_TO_ADDR(__BASE_ADDR, __HEADER_TYPE, __FIELD)   (CLEAR_VAL_SIMPLE(__BASE_ADDR, 32, PCI_CONFIG_ADDR_FUNCTION_NUMBER_BEGIN) | PCI_CONFIG_ADDR_REGISTER_OFFSET_TO_NUMBER(PCI_HEADER_FIELD_BEGIN(__HEADER_TYPE, __FIELD)))
#define PCI_HEADER_FIELD_TO_BEGIN_IN_REGISTER(__HEADER_TYPE, __FIELD)   (TRIM_VAL_SIMPLE(PCI_HEADER_FIELD_BEGIN(__HEADER_TYPE, __FIELD), 8, 2) * 8)
#define PCI_HEADER_FIELD_TO_END_IN_REGISTER(__HEADER_TYPE, __FIELD)     ((TRIM_VAL_SIMPLE(PCI_HEADER_FIELD_BEGIN(__HEADER_TYPE, __FIELD), 8, 2) + PCI_HEADER_FIELD_LENGTH(__HEADER_TYPE, __FIELD)) * 8)

#define PCI_HEADER_READ(__BASE_ADDR, __HEADER_TYPE, __FIELD)   EXTRACT_VAL(     \
    pci_read(PCI_HEADER_FIELD_TO_ADDR(__BASE_ADDR, __HEADER_TYPE, __FIELD)),    \
    32,                                                                         \
    PCI_HEADER_FIELD_TO_BEGIN_IN_REGISTER(__HEADER_TYPE, __FIELD),              \
    PCI_HEADER_FIELD_TO_END_IN_REGISTER(__HEADER_TYPE, __FIELD)                 \
)

#define PCI_HEADER_WRITE(__BASE_ADDR, __HEADER_TYPE, __FIELD, __VAL)   pci_write(           \
    PCI_HEADER_FIELD_TO_ADDR(__BASE_ADDR, __HEADER_TYPE, __FIELD),                          \
    CLEAR_VAL_RANGE(                                                                        \
        pci_read(PCI_HEADER_FIELD_TO_ADDR(__BASE_ADDR, __HEADER_TYPE, __FIELD)),            \
        32,                                                                                 \
        PCI_HEADER_FIELD_TO_BEGIN_IN_REGISTER(__HEADER_TYPE, __FIELD),                      \
        PCI_HEADER_FIELD_TO_END_IN_REGISTER(__HEADER_TYPE, __FIELD)                         \
    ) | VAL_LEFT_SHIFT(                                                                     \
        TRIM_VAL_SIMPLE(__VAL, 32, PCI_HEADER_FIELD_LENGTH(__HEADER_TYPE, __FIELD) * 8),    \
        PCI_HEADER_FIELD_TO_BEGIN_IN_REGISTER(__HEADER_TYPE, __FIELD)                       \
    )                                                                                       \
)

Result* pci_init();

bool pci_checkExist();

typedef struct PCIdevice {
    Uint32 baseAddr;
    Uint16 deviceID;
    Uint16 vendorID;
    Uint8 class;
    Uint8 subClass;
} PCIdevice;

Uint32 pci_getDeviceNum();

PCIdevice* pci_getDevice(Index32 index);

#endif // __DEVICES_BUS_PCI_H
