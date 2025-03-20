#if !defined(__DEVICES_BUS_PCI_H)
#define __DEVICES_BUS_PCI_H

typedef struct PCIcommonHeader PCIcommonHeader;
typedef struct PCIHeaderType0 PCIHeaderType0;
typedef struct PCIHeaderType1 PCIHeaderType1;
typedef struct PCIHeaderType2 PCIHeaderType2;
typedef struct PCIdevice PCIdevice;
typedef struct pciMSIXtableEntry pciMSIXtableEntry;
typedef struct pciMSIX pciMSIX;

#include<debug.h>
#include<kit/bit.h>
#include<kit/macro.h>
#include<kit/types.h>
#include<kit/util.h>
#include<real/ports/pci.h>
#include<real/simpleAsmLines.h>

#define PCI_MAX_REGISTER_NUM    POWER_2(PCI_CONFIG_ADDR_REGISTER_NUMBER_LENGTH)
#define PCI_MAX_FUNCTION_NUM    POWER_2(PCI_CONFIG_ADDR_FUNCTION_NUMBER_LENGTH)
#define PCI_MAX_DEVICE_NUM      POWER_2(PCI_CONFIG_ADDR_DEVICE_NUMBER_LENGTH)
#define PCI_MAX_BUS_NUM         POWER_2(PCI_CONFIG_ADDR_BUS_NUMBER_LENGTH)

typedef struct PCIcommonHeader {
    Uint16 vendorID;
#define PCI_COMMON_HEADER_INVALID_VENDOR_ID 0xFFFF
    Uint16 deviceID;
    Uint16 command;
#define PCI_COMMON_HEADER_COMMAND_FLAG_IO_SPACE                         FLAG16(0)
#define PCI_COMMON_HEADER_COMMAND_FLAG_MEMORY_SPACE                     FLAG16(1)
#define PCI_COMMON_HEADER_COMMAND_FLAG_BUS_MASTER                       FLAG16(2)
#define PCI_COMMON_HEADER_COMMAND_FLAG_SPECIAL_CYCLES                   FLAG16(3)
#define PCI_COMMON_HEADER_COMMAND_FLAG_MEMORY_WRITE_AND_INVALITE_ENABLE FLAG16(4)
#define PCI_COMMON_HEADER_COMMAND_FLAG_VGA_PALETTE_SNOOP                FLAG16(5)
#define PCI_COMMON_HEADER_COMMAND_FLAG_PARITY_ERROR_RESPONSE            FLAG16(6)
#define PCI_COMMON_HEADER_COMMAND_FLAG_SERR_ENABLE                      FLAG16(8)
#define PCI_COMMON_HEADER_COMMAND_FLAG_FAST_BACK_TO_BACK_ENABLE         FLAG16(9)
#define PCI_COMMON_HEADER_COMMAND_FLAG_INTERRUPT_DISABLE                FLAG16(10)
    Uint16 status;
#define PCI_COMMON_HEADER_STATUS_FLAG_INTERRUPT                         FLAG16(3)
#define PCI_COMMON_HEADER_STATUS_FLAG_CAPABILITIES_LIST                 FLAG16(4)
#define PCI_COMMON_HEADER_STATUS_FLAG_66MHZ_CAPABLE                     FLAG16(5)
#define PCI_COMMON_HEADER_STATUS_FLAG_FAST_BACK_TO_BACK_CAPABLE         FLAG16(6)
#define PCI_COMMON_HEADER_STATUS_FLAG_MASTER_DATA_PARITY_ERROR          FLAG16(7)
#define PCI_COMMON_HEADER_STATUS_DEVSEL_TIMING_FAST                     VAL_LEFT_SHIFT(0, 8)
#define PCI_COMMON_HEADER_STATUS_DEVSEL_TIMING_MEDIUM                   VAL_LEFT_SHIFT(1, 8)
#define PCI_COMMON_HEADER_STATUS_DEVSEL_TIMING_SLOW                     VAL_LEFT_SHIFT(2, 8)
#define PCI_COMMON_HEADER_STATUS_FLAG_SIGNALED_TARGET_ABORT             FLAG16(11)
#define PCI_COMMON_HEADER_STATUS_FLAG_RECEIVED_TARGET_ABORT             FLAG16(12)
#define PCI_COMMON_HEADER_STATUS_FLAG_RECEIVED_MASTER_ABORT             FLAG16(13)
#define PCI_COMMON_HEADER_STATUS_FLAG_SIGNALED_SYSTEM_ERROR             FLAG16(14)
#define PCI_COMMON_HEADER_STATUS_FLAG_DETECTDE_PARITY_ERROR             FLAG16(15)
    Uint8 revisionID;
    Uint8 progIF;       //Programming Interface Byte
    Uint8 subclass;     //Specifies the specific function the device performs
    Uint8 classCode;    //Specifies the type of function the device performs
#define PCI_COMMON_HEADER_CLASS_CODE_DEVICE_BUILT_BEFORE_CLASS_DEFINITIONS_FINALIZED    0x00
#define PCI_COMMON_HEADER_CLASS_CODE_MASS_STORAGE_CONTROLLER                            0x01
#define PCI_COMMON_HEADER_CLASS_CODE_NETWORK_CONTROLLER                                 0x02
#define PCI_COMMON_HEADER_CLASS_CODE_DISPLAY_CONTROLLER                                 0x03
#define PCI_COMMON_HEADER_CLASS_CODE_MULTIMEDIA_DEVICE                                  0x04
#define PCI_COMMON_HEADER_CLASS_CODE_MEMORY_CONTROLLER                                  0x05
#define PCI_COMMON_HEADER_CLASS_CODE_BRIDGE_CONTROLLER                                  0x06
#define PCI_COMMON_HEADER_CLASS_CODE_SIMPLE_COMMUNICATION_CONTROLLER                    0x07
#define PCI_COMMON_HEADER_CLASS_CODE_BASE_SYSTEM_PERIPHERALS                            0x08
#define PCI_COMMON_HEADER_CLASS_CODE_INPUT_DEVICE                                       0x09
#define PCI_COMMON_HEADER_CLASS_CODE_DOCKING_STATION                                    0x0A
#define PCI_COMMON_HEADER_CLASS_CODE_PROCESSORS                                         0x0B
#define PCI_COMMON_HEADER_CLASS_CODE_SERIAL_BUS_CONTROLLERS                             0x0C
#define PCI_COMMON_HEADER_CLASS_CODE_WIRELESS_CONTROLLER                                0x0D
#define PCI_COMMON_HEADER_CLASS_CODE_INTELLIGENT_IO_CONTROLLERS                         0x0E
#define PCI_COMMON_HEADER_CLASS_CODE_SATELLITE_COMMUNICATION_CONTROLLERS                0x0F
#define PCI_COMMON_HEADER_CLASS_CODE_EN_DECRYPTION_CONTROLLERS                          0x10
#define PCI_COMMON_HEADER_CLASS_CODE_DATA_ACQUISITION_AND_SIGNAL_PROCESSING_CONTROLLERS 0x11
#define PCI_COMMON_HEADER_CLASS_CODE_DEVICE_NOT_FIT_ANY_CLASS                           0xFF
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
#define PCI_CAPABILITY_ID_RESERVED                              0x00
#define PCI_CAPABILITY_ID_PCI_POWER_MANAGEMENT_INTERFACE        0x01
#define PCI_CAPABILITY_ID_AGP                                   0x02
#define PCI_CAPABILITY_ID_VPD                                   0x03
#define PCI_CAPABILITY_ID_SLOT_IDENTIFICATION                   0x04
#define PCI_CAPABILITY_ID_MESSAGE_SIGNALED_INTERRUPTS           0x05
#define PCI_CAPABILITY_ID_COMPACT_PCI_HOT_SWAP                  0x06
#define PCI_CAPABILITY_ID_PCIX                                  0x07
#define PCI_CAPABILITY_ID_HYPER_TRANSPORT                       0x08
#define PCI_CAPABILITY_ID_VENDOR_SPECIFIC                       0x09
#define PCI_CAPABILITY_ID_DEBUG_PORT                            0x0A
#define PCI_CAPABILITY_ID_COMPACT_PCI_CENTRAL_RESOURCE_CONTROL  0x0B
#define PCI_CAPABILITY_ID_PCI_HOT_PLUG                          0x0C
#define PCI_CAPABILITY_ID_PCI_BRIDGE_SUBSYSTEM_VENDOR_ID        0x0D
#define PCI_CAPABILITY_ID_AGP_8X                                0x0E
#define PCI_CAPABILITY_ID_SECURE_DEVICE                         0x0F
#define PCI_CAPABILITY_ID_PCI_EXPRESS                           0x10
#define PCI_CAPABILITY_ID_MSIX                                  0x11
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

#define PCI_GET_BAR_ADDR(__BASE_ADDR, __BAR_INDEX)  (PCI_HEADER_FIELD_TO_ADDR(__BASE_ADDR, PCIHeaderType0, bar0) + sizeof(Uint32) * (__BAR_INDEX))

void pci_init();

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

Uint32 pci_getBARaddr(Uint32 baseAddr, Uint8 barIndex);

Uint32 pci_readBAR(Uint32 baseAddr, Uint8 barIndex);

Uint32 pci_readBARspaceLength(Uint32 baseAddr, Uint8 barIndex);

Uint8 pci_findCapacityOffset(Uint32 baseAddr, Uint8 targetCapabilityID);

typedef struct pciMSIXtableEntry {
    Uint32 messageAddrLow;
    Uint32 messageAddrHigh;
    Uint32 messageData;
    Uint32 vectorControl;
} pciMSIXtableEntry;

typedef struct pciMSIX {
    Uint8 offset;
    Uint16 capacity;
    pciMSIXtableEntry* msixTable;
    Uint64* msixPBA;
} pciMSIX;

void pciMSIX_initStruct(pciMSIX* msix, Uint32 baseAddr);

void pciMSIX_addEntry(pciMSIX* msix, Index16 msixVec, int cpuID, int interruptVec, bool edgetrigger, bool deassert);

void pciMSIX_setMasked(pciMSIX* msix, Uint32 baseAddr, bool masked);

#endif // __DEVICES_BUS_PCI_H
