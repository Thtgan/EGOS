#include<devices/ata/ata.h>

#include<devices/ata/channel.h>
#include<devices/ata/pio.h>
#include<devices/blockDevice.h>
#include<devices/device.h>
#include<devices/partitionBlockDevice.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<real/simpleAsmLines.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<lib/errorPosix.h>

static ATAdeviceType __ata_getDeviceType(ATAchannel* channel, int deviceSelect);

static bool __ata_initDevice(ATAchannel* channel, Uint8 deviceSelect, ATAdevice* device);

static void __ata_identifyDevice(ATAchannel* channel, void* buffer);

static void __atapi_identifyDevice(ATAchannel* channel, void* buffer);

static Uint16 _ata_defauleChannelPortBases[2] = {
    0x1F0, 0x170
};

static void __ata_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN);

static void __ata_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN);

static void __ata_flush(Device* device);

static DeviceOperations _ata_deviceOperations = (DeviceOperations) {
    .readUnits  = __ata_readUnits,
    .writeUnits = __ata_writeUnits,
    .flush      = __ata_flush
};

static ATAdevice _ata_devices[4];
static ATAchannel _ata_channels[2];

void ata_initDevices() {
    ATAchannel dummy1;
    ATAdevice dummy2;

    MajorDeviceID major = device_allocMajor();
    ERROR_THROW_NEW_IF(major == DEVICE_INVALID_ID, ERROR_OUT_OF_RESOURCES, error_out);

    for (int i = 0; i < 2; ++i) {
        Uint16 portBase = _ata_defauleChannelPortBases[i];

        memory_memset(&dummy1, 0, sizeof(ATAchannel));
        dummy1.portBase = portBase;
        dummy1.deviceSelect = -1;

        outb(ATA_REGISTER_CONTROL(portBase), ATA_CONTROL_NO_INTERRUPT);

        bool hasDevice = false;
        for (int j = 0; j < 2; ++j) {
            if (!__ata_initDevice(&dummy1, j, &dummy2)) {
                continue;
            }

            ATAdevice* device = _ata_devices + ((i << 1) | j);
            memory_memcpy(device, &dummy2, sizeof(ATAdevice));

            dummy1.devices[j] = device;

            hasDevice = true;
        }

        if (!hasDevice) {
            continue;
        }

        ATAchannel* channel = _ata_channels + i;
        memory_memcpy(channel, &dummy1, sizeof(ATAchannel));

        ata_channel_reset(channel);

        for (int j = 0; j < 2; ++j) {
            ATAdevice* ataDevice = channel->devices[j];
            if (ataDevice == NULL) {
                continue;
            }

            memory_memset(ataDevice->name, 0, sizeof(ataDevice->name));
            print_snprintf(ataDevice->name, sizeof(ataDevice->name), "HD%c", 'A' + (i << 1) + j);
            ataDevice->channel = channel;
            ataDevice->type = __ata_getDeviceType(channel, j);

            MinorDeviceID minor = device_allocMinor(major);
            ERROR_THROW_NEW_IF(minor == DEVICE_INVALID_ID, ERROR_OUT_OF_RESOURCES, error_out);

            BlockDeviceInitArgs args = {
                .deviceInitArgs     = (DeviceInitArgs) {
                    .id             = DEVICE_BUILD_ID(major, minor),
                    .name           = ataDevice->name,
                    .parent         = NULL,
                    .granularity    = BLOCK_DEVICE_DEFAULT_BLOCK_SIZE_SHIFT,
                    .capacity       = ataDevice->sectorNum,
                    .flags          = DEVICE_FLAGS_BUFFERED,
                    .operations     = &_ata_deviceOperations,
                },
            };

            BlockDevice* blockDevice = &ataDevice->blockDevice;
            blockDevice_initStruct(blockDevice, &args);
            CHECK_ERROR(error_out);

            device_registerDevice(&blockDevice->device);
            CHECK_ERROR(error_out);

            if (ataDevice->sectorNum == INFINITE) { //TODO: A more proper way to deal with CD-ROM
                continue;
            }

            partitionBlockDevice_probePartitions(blockDevice);
            CHECK_ERROR(error_out);
        }
    }

    return;
error_out:
}

void ata_sendCommand(ATAchannel* channel, ATAcommand* command) {
    Uint16 portBase = channel->portBase;
    ERROR_THROW_NEW_IF(TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY), ERROR_IO_FAILED, error_out);

    outb(ATA_REGISTER_DEVICE(portBase), VAL_OR(CLEAR_VAL(command->device, ATA_DEVICE_DEVICE1), channel->deviceSelect == 0 ? ATA_DEVICE_DEVICE0 : ATA_DEVICE_DEVICE1));
    ATA_DELAY_400NS(portBase);

    ERROR_THROW_NEW_IF(TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY), ERROR_IO_FAILED, error_out);

    outb(ATA_REGISTER_FEATURE(portBase)         , command->feature      );
    outb(ATA_REGISTER_SECTOR_COUNT(portBase)    , command->sectorCount  );
    outb(ATA_REGISTER_ADDR1(portBase)           , command->addr1        );
    outb(ATA_REGISTER_ADDR2(portBase)           , command->addr2        );
    outb(ATA_REGISTER_ADDR3(portBase)           , command->addr3        );

    outb(ATA_REGISTER_COMMAND(portBase)         , command->command      );

    return;
error_out:
}

#define __ATA_WAIT_RETRY_TIME   65535

Flags8 ata_waitTillClear(Uint16 channelPortBase, Flags8 waitFlags) {
    Uint16 retry = __ATA_WAIT_RETRY_TIME;
    Flags8 status;
    do {
        ATA_DELAY_400NS(channelPortBase);
        status = inb(ATA_REGISTER_ALT_STATUS(channelPortBase));
    } while (TEST_FLAGS_CONTAIN(status, waitFlags) && retry-- != 0);
    return status;
}

Flags8 ata_waitTillSet(Uint16 channelPortBase, Flags8 waitFlags) {
    Uint16 retry = __ATA_WAIT_RETRY_TIME;
    Flags8 status;
    do {
        ATA_DELAY_400NS(channelPortBase);
        status = inb(ATA_REGISTER_ALT_STATUS(channelPortBase));
    } while (TEST_FLAGS_NONE(status, waitFlags) && retry-- != 0);
    return status;
}

void ata_waitForData(Uint16 channelPortBase) {
    Flags8 status = ata_waitTillClear(channelPortBase, ATA_STATUS_FLAG_BUSY);
    ERROR_THROW_NEW_IF(TEST_FLAGS_CONTAIN(status, ATA_STATUS_FLAG_BUSY | ATA_STATUS_FLAG_ERROR), ERROR_IO_FAILED, error_out);

    ERROR_THROW_NEW_IF(TEST_FLAGS_FAIL(status, ATA_STATUS_FLAG_DATA_REQUIRE_SERVICE), ERROR_IO_FAILED, error_out);

    return;
error_out:
}

static ATAdeviceType __ata_getDeviceType(ATAchannel* channel, int deviceSelect) {
    ata_channel_selectDevice(channel, deviceSelect);

    Uint16 portBase = channel->portBase;
    Uint8 sig1, sig2, sig3, sig4;
    sig1 = inb(ATA_REGISTER_SECTOR_COUNT(portBase)), sig2 = inb(ATA_REGISTER_ADDR1(portBase));
    if (!(sig1 == 0x01 && sig2 == 0x01)) {
        return ATA_DEVICE_TYPE_UNKNOWN;
    }

    sig3 = inb(ATA_REGISTER_ADDR2(portBase)), sig4 = inb(ATA_REGISTER_ADDR3(portBase));
    if (sig3 == 0x00 && sig4 == 0x00) {
        return ATA_DEVICE_TYPE_PATA;
    } else if (sig3 == 0x14 && sig4 == 0xEB) {
        return ATA_DEVICE_TYPE_PATA_API;
    } else if (sig3 == 0x3C && sig4 == 0xC3) {
        return ATA_DEVICE_TYPE_SATA;
    } else if (sig3 == 0x69 && sig4 == 0x96) {
        return ATA_DEVICE_TYPE_SATA_API;
    }

    return ATA_DEVICE_TYPE_UNKNOWN;
}

static bool __ata_initDevice(ATAchannel* channel, Uint8 deviceSelect, ATAdevice* device) {
    Uint16 portBase = channel->portBase;
    void* buffer = NULL;

    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        goto error_out;
    }

    ata_channel_selectDevice(channel, deviceSelect);

    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        goto error_out;
    }

    outb(ATA_REGISTER_SECTOR_COUNT(portBase), 0x55);
    outb(ATA_REGISTER_ADDR1(portBase), 0xAA);
    outb(ATA_REGISTER_SECTOR_COUNT(portBase), 0xAA);
    outb(ATA_REGISTER_ADDR1(portBase), 0x55);
    outb(ATA_REGISTER_SECTOR_COUNT(portBase), 0x55);
    outb(ATA_REGISTER_ADDR1(portBase), 0xAA);

    if (inb(ATA_REGISTER_SECTOR_COUNT(portBase)) != 0x55 ||
        inb(ATA_REGISTER_ADDR1(portBase)) != 0xAA ||
        inb(ATA_REGISTER_DEVICE(portBase)) != (deviceSelect ? ATA_DEVICE_DEVICE1 : ATA_DEVICE_DEVICE0)) {
        goto error_out;
    }

    buffer = mm_allocate(BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);
    if (buffer == NULL) {
        goto error_out;
    }

    __atapi_identifyDevice(channel, buffer);
    bool isAtapi = !error_pending();
    error_clear();

    if (!isAtapi) {
        __ata_identifyDevice(channel, buffer);
    }

    if (error_pending()) {
        goto error_out;
    }

    if (isAtapi) {
        device->sectorNum = INFINITE;
    } else {
        device->sectorNum = ((ATAdeviceIdentify*)buffer)->commandSetSupport.lba48Supported ?
                            ((ATAdeviceIdentify*)buffer)->maxUserLBAfor48bitAddress :
                            ((ATAdeviceIdentify*)buffer)->addressableSectorNum;
    }
    device->deviceNumber = deviceSelect;

    mm_free(buffer);
    return true;
error_out:
    if (buffer != NULL) {
        mm_free(buffer);
    }
    error_clear();
    return false;
}

static void __ata_identifyDevice(ATAchannel* channel, void* buffer) {
    Uint16 portBase = channel->portBase;
    ERROR_THROW_NEW_IF(TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY), ERROR_IO_FAILED, error_out);

    ATAcommand command;
    memory_memset(&command, 0, sizeof(ATAcommand));
    command.command = ATA_COMMAND_IDENTIFY_DEVICE;

    ata_sendCommand(channel, &command);
    CHECK_ERROR(error_out);

    ata_pio_readBlocks(portBase, 1, buffer);
    CHECK_ERROR(error_out);

    return;
error_out:
}

static void __atapi_identifyDevice(ATAchannel* channel, void* buffer) {
    Uint16 portBase = channel->portBase;
    ERROR_THROW_NEW_IF(TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY), ERROR_IO_FAILED, error_out);

    ATAcommand command;
    memory_memset(&command, 0, sizeof(ATAcommand));
    command.command = ATA_COMMAND_IDENTIFY_PACKET_DEVICE;

    ata_sendCommand(channel, &command);
    CHECK_ERROR(error_out);

    ata_pio_readBlocks(portBase, 1, buffer);
    CHECK_ERROR(error_out);

    return;
error_out:
}

static void __ata_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN) {    
    ATAdevice* ataDevice = HOST_POINTER(device, ATAdevice, blockDevice.device);

    Index32 LBA28 = unitIndex;
    ATAcommand command = {
        .command = ATA_COMMAND_READ_SECTORS,
        .device = (ataDevice->channel->devices[0] == ataDevice ? ATA_DEVICE_DEVICE0 : ATA_DEVICE_DEVICE1) | ATA_DEVICE_LBA | EXTRACT_VAL(LBA28, 32, 24, 28),
        .feature = 0,
        .sectorCount = unitN,
        .addr1 = EXTRACT_VAL(LBA28, 32, 0, 8),
        .addr2 = EXTRACT_VAL(LBA28, 32, 8, 16),
        .addr3 = EXTRACT_VAL(LBA28, 32, 16, 24),
    };
    return ata_pio_readData(ataDevice, &command, buffer);
}

static void __ata_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN) {
    ATAdevice* ataDevice = HOST_POINTER(device, ATAdevice, blockDevice.device);

    Index32 LBA28 = unitIndex;
    ATAcommand command = {
        .command = ATA_COMMAND_WRITE_SECTORS,
        .device = (ataDevice->channel->devices[0] == ataDevice ? ATA_DEVICE_DEVICE0 : ATA_DEVICE_DEVICE1) | ATA_DEVICE_LBA | EXTRACT_VAL(LBA28, 32, 24, 28),
        .feature = 0,
        .sectorCount = unitN,
        .addr1 = EXTRACT_VAL(LBA28, 32, 0, 8),
        .addr2 = EXTRACT_VAL(LBA28, 32, 8, 16),
        .addr3 = EXTRACT_VAL(LBA28, 32, 16, 24),
    };
    return ata_pio_writeData(ataDevice, &command, buffer);
}

static void __ata_flush(Device* device) {
    //TODO: Maybe more procedure?
}