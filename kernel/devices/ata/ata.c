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
#include<error.h>

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
    if (major == DEVICE_INVALID_ID) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

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

            MajorDeviceID minor = device_allocMinor(major);
            if (minor == DEVICE_INVALID_ID) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

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
            ERROR_GOTO_IF_ERROR(0);

            device_registerDevice(&blockDevice->device);
            ERROR_GOTO_IF_ERROR(0);

            if (ataDevice->sectorNum == INFINITE) { //TODO: A more proper way to deal with CD-ROM
                continue;
            }

            partitionBlockDevice_probePartitions(blockDevice);
            ERROR_GOTO_IF_ERROR(0);
        }
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void ata_sendCommand(ATAchannel* channel, ATAcommand* command) {
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    outb(ATA_REGISTER_DEVICE(portBase), VAL_OR(CLEAR_VAL(command->device, ATA_DEVICE_DEVICE1), channel->deviceSelect == 0 ? ATA_DEVICE_DEVICE0 : ATA_DEVICE_DEVICE1));
    ATA_DELAY_400NS(portBase);

    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    outb(ATA_REGISTER_FEATURE(portBase)         , command->feature      );
    outb(ATA_REGISTER_SECTOR_COUNT(portBase)    , command->sectorCount  );
    outb(ATA_REGISTER_ADDR1(portBase)           , command->addr1        );
    outb(ATA_REGISTER_ADDR2(portBase)           , command->addr2        );
    outb(ATA_REGISTER_ADDR3(portBase)           , command->addr3        );

    outb(ATA_REGISTER_COMMAND(portBase)         , command->command      );

    return;
    ERROR_FINAL_BEGIN(0);
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
    if (TEST_FLAGS_CONTAIN(status, ATA_STATUS_FLAG_BUSY | ATA_STATUS_FLAG_ERROR)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    if (TEST_FLAGS_FAIL(status, ATA_STATUS_FLAG_DATA_REQUIRE_SERVICE)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
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
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    ata_channel_selectDevice(channel, deviceSelect);

    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    outb(ATA_REGISTER_SECTOR_COUNT(portBase),   0x55);
    outb(ATA_REGISTER_ADDR1(portBase),          0xAA);
    outb(ATA_REGISTER_SECTOR_COUNT(portBase),   0xAA);
    outb(ATA_REGISTER_ADDR1(portBase),          0x55);
    outb(ATA_REGISTER_SECTOR_COUNT(portBase),   0x55);
    outb(ATA_REGISTER_ADDR1(portBase),          0xAA);

    if (inb(ATA_REGISTER_SECTOR_COUNT(portBase)) != 0x55 || inb(ATA_REGISTER_ADDR1(portBase)) != 0xAA && inb(ATA_REGISTER_DEVICE(portBase)) != (deviceSelect ? ATA_DEVICE_DEVICE1 : ATA_DEVICE_DEVICE0)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    void* buffer = mm_allocate(BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    do {
        bool skip = true;
        __atapi_identifyDevice(channel, buffer);
        ERROR_CHECKPOINT({
            ERROR_CLEAR();
            skip = false;
        });

        if (skip) {
            device->sectorNum = INFINITE;
            break;
        }
        skip = true;
        
        __ata_identifyDevice(channel, buffer);
        ERROR_CHECKPOINT({
            ERROR_CLEAR();
            skip = false;
        });

        if (skip) {
            ATAdeviceIdentify* data = (ATAdeviceIdentify*)buffer;
            device->sectorNum = data->commandSetSupport.lba48Supported ? data->maxUserLBAfor48bitAddress : data->addressableSectorNum;
            break;
        }

        ERROR_GOTO(1);
    } while (0);

    device->deviceNumber = deviceSelect;

    mm_free(buffer);
    return true;
    ERROR_FINAL_BEGIN(0);
    return false;

    ERROR_FINAL_BEGIN(1);
    mm_free(buffer);
    ERROR_GOTO(0);
}

static void __ata_identifyDevice(ATAchannel* channel, void* buffer) {
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    ATAcommand command;
    memory_memset(&command, 0, sizeof(ATAcommand));
    command.command = ATA_COMMAND_IDENTIFY_DEVICE;

    ata_sendCommand(channel, &command);
    ERROR_GOTO_IF_ERROR(0);

    ata_pio_readBlocks(portBase, 1, buffer);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __atapi_identifyDevice(ATAchannel* channel, void* buffer) {
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    ATAcommand command;
    memory_memset(&command, 0, sizeof(ATAcommand));
    command.command = ATA_COMMAND_IDENTIFY_PACKET_DEVICE;

    ata_sendCommand(channel, &command);
    ERROR_GOTO_IF_ERROR(0);

    ata_pio_readBlocks(portBase, 1, buffer);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
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