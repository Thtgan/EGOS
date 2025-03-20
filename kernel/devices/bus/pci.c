#include<devices/bus/pci.h>

#include<kit/types.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<real/ports/pci.h>
#include<real/simpleAsmLines.h>
#include<structs/vector.h>
#include<error.h>

static void __pci_probe();

static void __pci_probeBus(Uint8 bus);

static void __pci_probeDevice(Uint8 bus, Uint8 device);

static bool __pci_probeFunction(Uint8 bus, Uint8 device, Uint8 function);

static Vector _pci_devices;

static void __pci_readDevice(Uint32 baseAddr, PCIdevice* device);

static void __pci_addDevice(PCIdevice* device);

void pci_init() {
    vector_initStruct(&_pci_devices);

    if (pci_checkExist()) {
        __pci_probe();
    } else {
        print_debugPrintf("PCI not supported\n");
    }
}

bool pci_checkExist() {
    outl(PCI_CONFIG_ADDR, PCI_CONFIG_ADDR_ENABLE);
    return inl(PCI_CONFIG_ADDR) == PCI_CONFIG_ADDR_ENABLE;
}

Uint32 pci_getDeviceNum() {
    return _pci_devices.size;
}

PCIdevice* pci_getDevice(Index32 index) {
    Object ret = vector_get(&_pci_devices, index);
    if (ret == OBJECT_NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    return (PCIdevice*)ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

Uint32 pci_getBARaddr(Uint32 baseAddr, Uint8 barIndex) {
    if (barIndex > 5) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    return PCI_HEADER_FIELD_TO_ADDR(baseAddr, PCIHeaderType0, bar0) + barIndex * sizeof(Uint32);
    ERROR_FINAL_BEGIN(0);
    return (Uint32)-1;
}

Uint32 pci_readBAR(Uint32 baseAddr, Uint8 barIndex) {
    Uint32 addr = pci_getBARaddr(baseAddr, barIndex);
    if (addr == (Uint32)-1) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    Uint32 ret = pci_read(addr);
    ret = ret & (TEST_FLAGS(ret, FLAG8(0)) ? 0xFFFFFFFC : 0xFFFFFFF0);
    
    return ret;
    ERROR_FINAL_BEGIN(0);
    return (Uint32)-1;
}

Uint32 pci_readBARspaceLength(Uint32 baseAddr, Uint8 barIndex) {
    Uint32 addr = pci_getBARaddr(baseAddr, barIndex);
    if (addr == (Uint32)-1) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Uint32 origin = pci_read(addr);
    pci_write(addr, 0xFFFFFFFF);
    Uint32 ret = pci_read(addr) & (TEST_FLAGS(origin, FLAG8(0)) ? 0xFFFFFFFC : 0xFFFFFFF0);
    pci_write(addr, origin);
    ret = -ret;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return (Uint32)-1;
}

#define __PCI_END_OF_CAPABILITY_LIST 0x00

Uint8 pci_findCapacityOffset(Uint32 baseAddr, Uint8 targetCapabilityID) {
    if (TEST_FLAGS_FAIL(PCI_HEADER_READ(baseAddr, struct PCIHeaderType0, commonHeader.status), PCI_COMMON_HEADER_STATUS_FLAG_CAPABILITIES_LIST)) {
        return 0;
    }

    Uint8 offset = PCI_HEADER_READ(baseAddr, struct PCIHeaderType0, capabilitiesPtr);
    while (offset != __PCI_END_OF_CAPABILITY_LIST) {
        Uint8 capabilityID = pci_read(baseAddr | offset) & 0xFF;
        if (capabilityID == targetCapabilityID) {
            return offset;
        }

        offset = pci_read(baseAddr | (offset + 1)) & 0xFF;
    }

    return 0;
}

void pciMSIX_initStruct(pciMSIX* msix, Uint32 baseAddr) {
    msix->offset = pci_findCapacityOffset(baseAddr, PCI_CAPABILITY_ID_MSIX);
    if (msix->offset == 0 || (msix->offset & 3) != 0) {
        ERROR_THROW(ERROR_ID_DATA_ERROR, 0);
    }

    Uint32 msgctl = pci_read(baseAddr | msix->offset);
    ERROR_GOTO_IF_ERROR(0);
    msix->capacity = EXTRACT_VAL(msgctl, 32, 16, 26) + 1;
    SET_FLAG_BACK(msgctl, FLAG32(31) | FLAG32(30));
    pci_write(baseAddr | msix->offset, msgctl);

    Uint32 dword = pci_read(baseAddr | (msix->offset + 4));
    ERROR_GOTO_IF_ERROR(0);
    int bir = dword & 0x7, offset = dword & 0xFFFFFFF8;
    msix->msixTable = (pciMSIXtableEntry*)(Uintptr)(pci_readBAR(baseAddr, bir) + offset);

    dword = pci_read(baseAddr | (msix->offset + 8));
    ERROR_GOTO_IF_ERROR(0);
    bir = dword & 0x7, offset = dword & 0xFFFFFFF8;
    msix->msixPBA = (Uint64*)(Uintptr)(pci_readBAR(baseAddr, bir) + offset);

    Uint16 command = PCI_HEADER_READ(baseAddr, struct PCIHeaderType0, commonHeader.command);
    SET_FLAG_BACK(command, PCI_COMMON_HEADER_COMMAND_FLAG_INTERRUPT_DISABLE);
    PCI_HEADER_WRITE(baseAddr, struct PCIHeaderType0, commonHeader.command, command);

    return;
    ERROR_FINAL_BEGIN(0);
}

void pciMSIX_addEntry(pciMSIX* msix, Index16 msixVec, int cpuID, int interruptVec, bool edgetrigger, bool deassert) {
    if (msixVec >= msix->capacity) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    pciMSIXtableEntry* entry = (pciMSIXtableEntry*)paging_convertAddressP2V(msix->msixTable + msixVec);

    Uint64 msixTableEntryAddr = 0xFEE00000 | VAL_LEFT_SHIFT(cpuID, 12);
    Uint32 msixTableEntryData = interruptVec + 32;
    if (!deassert) {
        SET_FLAG_BACK(msixTableEntryData, FLAG16(14));
    }

    if (!edgetrigger) {
        SET_FLAG_BACK(msixTableEntryData, FLAG16(15));
    }

    *entry = (pciMSIXtableEntry) {
        .messageAddrLow = EXTRACT_VAL(msixTableEntryAddr, 64, 0, 32),
        .messageAddrHigh = EXTRACT_VAL(msixTableEntryAddr, 64, 32, 64),
        .messageData = msixTableEntryData,
        .vectorControl = 0
    };

    return;
    ERROR_FINAL_BEGIN(0);
}

void pciMSIX_setMasked(pciMSIX* msix, Uint32 baseAddr, bool masked) {
    Uint32 msgctl = pci_read(baseAddr | msix->offset);

    if (masked) {
        SET_FLAG_BACK(msgctl, FLAG16(30));
    } else {
        CLEAR_FLAG_BACK(msgctl, FLAG16(30));
    }

    pci_write(baseAddr | msix->offset, msgctl);
}

static void __pci_probe() {
    for (Uint16 bus = 0; bus < PCI_MAX_BUS_NUM; ++bus) {
        __pci_probeBus(bus);
    }
}

static void __pci_probeBus(Uint8 bus) {
    for (Uint8 device = 0; device < PCI_MAX_DEVICE_NUM; ++device) {
        __pci_probeDevice(bus, device);
    }
}

static void __pci_probeDevice(Uint8 bus, Uint8 device) {
    Uint8 function = 0;
    if (!__pci_probeFunction(bus, device, function)) {
        ERROR_GOTO_IF_ERROR(0);
        return;
    }

    Uint32 baseAddr = pci_buildAddr(bus, device, function, 0);
    Uint8 headerType = PCI_HEADER_READ(baseAddr, PCIcommonHeader, headerType);
    if (TEST_FLAGS_FAIL(headerType, PCI_COMMON_HEADER_HEADER_TYPE_FLAG_MULTIPLE_FUNCTION)) {
        return;
    }

    for (function = 1; function < PCI_MAX_FUNCTION_NUM; ++function) {
        if (!__pci_probeFunction(bus, device, function)) {
            ERROR_GOTO_IF_ERROR(0);
            return;
        }
    }

    ERROR_FINAL_BEGIN(0);
    ERROR_CLEAR();
    return;
}

static bool __pci_probeFunction(Uint8 bus, Uint8 device, Uint8 function) {
    Uint32 baseAddr = pci_buildAddr(bus, device, function, 0);

    PCIdevice tmpDevice;
    __pci_readDevice(baseAddr, &tmpDevice);
    ERROR_GOTO_IF_ERROR(0);

    PCIdevice* pciDevice = memory_allocate(sizeof(PCIdevice));
    if (pciDevice == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(1);
    }

    memory_memcpy(pciDevice, &tmpDevice, sizeof(PCIdevice));
    __pci_addDevice(pciDevice);
    ERROR_GOTO_IF_ERROR(1);

    if (pciDevice->class == PCI_COMMON_HEADER_CLASS_CODE_BRIDGE && pciDevice->subClass == PCI_COMMON_HEADER_SUB_CALSS_PCI2PCI_BRIDGE) {
        __pci_probeBus(PCI_HEADER_READ(baseAddr, PCIHeaderType1, secondaryBusNumber));
    }

    return true;
    ERROR_FINAL_BEGIN(0);
    ERROR_CLEAR();
    return false;

    ERROR_FINAL_BEGIN(1);
    if (pciDevice != NULL) {
        memory_free(pciDevice);
    }
    return false;
}

static void __pci_readDevice(Uint32 baseAddr, PCIdevice* device) {
    Uint16 vendorID = PCI_HEADER_READ(baseAddr, PCIcommonHeader, vendorID);
    if (vendorID == PCI_COMMON_HEADER_INVALID_VENDOR_ID) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    device->baseAddr    = baseAddr;
    device->deviceID    = PCI_HEADER_READ(baseAddr, PCIcommonHeader, deviceID);
    device->vendorID    = vendorID;
    device->class       = PCI_HEADER_READ(baseAddr, PCIcommonHeader, classCode);
    device->subClass    = PCI_HEADER_READ(baseAddr, PCIcommonHeader, subclass);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __pci_addDevice(PCIdevice* device) {
    vector_push(&_pci_devices, (Object)device);  //TODO: Error passthrough here
}
