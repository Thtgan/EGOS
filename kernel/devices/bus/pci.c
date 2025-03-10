#include<devices/bus/pci.h>

#include<kit/types.h>
#include<memory/memory.h>
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
        print_printf("PCI not supported\n");
    }
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
