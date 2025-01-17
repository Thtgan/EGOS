#include<devices/bus/pci.h>

#include<kit/types.h>
#include<memory/memory.h>
#include<real/ports/pci.h>
#include<real/simpleAsmLines.h>
#include<structs/vector.h>
#include<result.h>

static void __pci_probe();

static void __pci_probeBus(Uint8 bus);

static void __pci_probeDevice(Uint8 bus, Uint8 device);

static OldResult __pci_probeFunction(Uint8 bus, Uint8 device, Uint8 function);

static Vector pci_devices;

static OldResult __pci_readDevice(Uint32 baseAddr, PCIdevice* device);

static OldResult __pci_addDevice(PCIdevice* device);

Result* pci_init() {
    vector_initStruct(&pci_devices);

    if (pci_checkExist()) {
        __pci_probe();
    } else {
        print_printf(TERMINAL_LEVEL_DEBUG, "PCI not supported\n");
    }

    ERROR_RETURN_OK();
}

bool pci_checkExist() {
    outl(PCI_CONFIG_ADDR, PCI_CONFIG_ADDR_ENABLE);
    return inl(PCI_CONFIG_ADDR) == PCI_CONFIG_ADDR_ENABLE;
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
    if (__pci_probeFunction(bus, device, function) != RESULT_SUCCESS) {
        return;
    }

    Uint32 baseAddr = pci_buildAddr(bus, device, function, 0);
    Uint8 headerType = PCI_HEADER_READ(baseAddr, PCIcommonHeader, headerType);
    if (TEST_FLAGS_FAIL(headerType, PCI_COMMON_HEADER_HEADER_TYPE_FLAG_MULTIPLE_FUNCTION)) {
        return;
    }

    for (function = 1; function < PCI_MAX_FUNCTION_NUM; ++function) {
        if (__pci_probeFunction(bus, device, function) != RESULT_SUCCESS) {
            break;
        }
    }
}

static OldResult __pci_probeFunction(Uint8 bus, Uint8 device, Uint8 function) {
    Uint32 baseAddr = pci_buildAddr(bus, device, function, 0);

    PCIdevice tmpDevice;
    if (__pci_readDevice(baseAddr, &tmpDevice) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    PCIdevice* pciDevice = memory_allocate(sizeof(PCIdevice));
    if (pciDevice == NULL) {
        return RESULT_ERROR;
    }

    memory_memcpy(pciDevice, &tmpDevice, sizeof(PCIdevice));
    if (__pci_addDevice(pciDevice) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    if (pciDevice->class == PCI_COMMON_HEADER_CLASS_CODE_BRIDGE && pciDevice->subClass == PCI_COMMON_HEADER_SUB_CALSS_PCI2PCI_BRIDGE) {
        __pci_probeBus(PCI_HEADER_READ(baseAddr, PCIHeaderType1, secondaryBusNumber));
    }

    return RESULT_SUCCESS;
}

Uint32 pci_getDeviceNum() {
    return pci_devices.size;
}

PCIdevice* pci_getDevice(Index32 index) {
    Object ret = OBJECT_NULL;
    return vector_get(&pci_devices, index, &ret) == RESULT_SUCCESS ? (PCIdevice*)ret : NULL;
}

static OldResult __pci_readDevice(Uint32 baseAddr, PCIdevice* device) {
    Uint16 vendorID = PCI_HEADER_READ(baseAddr, PCIcommonHeader, vendorID);
    if (vendorID == PCI_COMMON_HEADER_INVALID_VENDOR_ID) {
        return RESULT_ERROR;
    }

    device->baseAddr    = baseAddr;
    device->deviceID    = PCI_HEADER_READ(baseAddr, PCIcommonHeader, deviceID);
    device->vendorID    = vendorID;
    device->class       = PCI_HEADER_READ(baseAddr, PCIcommonHeader, classCode);
    device->subClass    = PCI_HEADER_READ(baseAddr, PCIcommonHeader, subclass);

    return RESULT_SUCCESS;
}

static OldResult __pci_addDevice(PCIdevice* device) {
    return vector_push(&pci_devices, (Object)device);
}
