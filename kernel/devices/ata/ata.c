#include<devices/ata/ata.h>

#include<devices/ata/channel.h>
#include<devices/ata/pio.h>
#include<devices/block/blockDevice.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<print.h>

static ATAdeviceType __getATAdeviceType(ATAchannel* channel, int deviceSelect);

static Result __initATADevice(ATAchannel* channel, Uint8 deviceSelect, ATAdevice* device);

static Result __identifyATAdevice(ATAchannel* channel, void* buffer);

static Result __identifyATAPIdevice(ATAchannel* channel, void* buffer);

static Uint16 _defauleChannelPortBases[2] = {
    0x1F0, 0x170
};

static Result __ATAreadBlocks(BlockDevice* this, Index64 blockIndex, void* buffer, Size n);

static Result __ATAwriteBlocks(BlockDevice* this, Index64 blockIndex, const void* buffer, Size n);

static BlockDeviceOperation _operations = {
    .readBlocks = __ATAreadBlocks,
    .writeBlocks = __ATAwriteBlocks
};

Result initATAdevices() {
    ATAchannel dummy1;
    ATAdevice dummy2;
    for (int i = 0; i < 2; ++i) {
        Uint16 portBase = _defauleChannelPortBases[i];

        memset(&dummy1, 0, sizeof(ATAchannel));
        dummy1.portBase = portBase;
        dummy1.deviceSelect = -1;

        outb(ATA_REGISTER_CONTROL(portBase), ATA_CONTROL_NO_INTERRUPT);

        bool hasDevice = false;
        for (int j = 0; j < 2; ++j) {
            if (__initATADevice(&dummy1, j, &dummy2) == RESULT_FAIL) {
                continue;
            }

            ATAdevice* device = kMalloc(sizeof(ATAdevice));
            if (device == NULL) {
                return RESULT_FAIL;
            }

            memcpy(device, &dummy2, sizeof(ATAdevice));

            dummy1.devices[j] = device;

            hasDevice = true;
        }

        if (!hasDevice) {
            continue;
        }

        ATAchannel* channel = kMalloc(sizeof(ATAchannel));
        if (channel == NULL) {
            return RESULT_FAIL;
        }

        memcpy(channel, &dummy1, sizeof(ATAchannel));

        ATA_resetChannel(channel);

        for (int j = 0; j < 2; ++j) {
            ATAdevice* ATAdevice = channel->devices[j];
            if (ATAdevice == NULL) {
                continue;
            }

            sprintf(ATAdevice->name, "HD%c", 'A' + (i << 1) + j);
            ATAdevice->channel = channel;
            ATAdevice->type = __getATAdeviceType(channel, j);

            BlockDevice* blockDevice = createBlockDevice(ATAdevice->name, ATAdevice->sectorNum, &_operations, (Object)ATAdevice);
            registerBlockDevice(blockDevice);
        }
    }

    return RESULT_SUCCESS;
}

Result ATA_sendCommand(ATAchannel* channel, ATAcommand* command) {
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    outb(ATA_REGISTER_DEVICE(portBase), VAL_OR(CLEAR_VAL(command->device, ATA_DEVICE_DEVICE1), channel->deviceSelect == 0 ? ATA_DEVICE_DEVICE0 : ATA_DEVICE_DEVICE1));
    ATA_DELAY_400NS(portBase);

    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    outb(ATA_REGISTER_FEATURE(portBase)         , command->feature      );
    outb(ATA_REGISTER_SECTOR_COUNT(portBase)    , command->sectorCount  );
    outb(ATA_REGISTER_ADDR1(portBase)           , command->addr1        );
    outb(ATA_REGISTER_ADDR2(portBase)           , command->addr2        );
    outb(ATA_REGISTER_ADDR3(portBase)           , command->addr3        );

    outb(ATA_REGISTER_COMMAND(portBase)         , command->command      );

    return RESULT_SUCCESS;
}

#define __ATA_WAIT_RETRY_TIME   65535

Flags8 ATA_waitTillClear(Uint16 channelPortBase, Flags8 waitFlags) {
    Uint16 retry = __ATA_WAIT_RETRY_TIME;
    Flags8 status;
    do {
        ATA_DELAY_400NS(channelPortBase);
        status = inb(ATA_REGISTER_ALT_STATUS(channelPortBase));
    } while (TEST_FLAGS_CONTAIN(status, waitFlags) && retry-- != 0);
    return status;
}

Flags8 ATA_waitTillSet(Uint16 channelPortBase, Flags8 waitFlags) {
    Uint16 retry = __ATA_WAIT_RETRY_TIME;
    Flags8 status;
    do {
        ATA_DELAY_400NS(channelPortBase);
        status = inb(ATA_REGISTER_ALT_STATUS(channelPortBase));
    } while (TEST_FLAGS_NONE(status, waitFlags) && retry-- != 0);
    return status;
}

Result ATA_waitForData(Uint16 channelPortBase) {
    Flags8 status = ATA_waitTillClear(channelPortBase, ATA_STATUS_FLAG_BUSY);
    if (TEST_FLAGS_CONTAIN(status, ATA_STATUS_FLAG_BUSY | ATA_STATUS_FLAG_ERROR)) {
        return RESULT_FAIL;
    }

    if (TEST_FLAGS_FAIL(status, ATA_STATUS_FLAG_DATA_REQUIRE_SERVICE)) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static ATAdeviceType __getATAdeviceType(ATAchannel* channel, int deviceSelect) {
    ATA_selectDevice(channel, deviceSelect);

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

static Result __initATADevice(ATAchannel* channel, Uint8 deviceSelect, ATAdevice* device) {
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    ATA_selectDevice(channel, deviceSelect);

    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    outb(ATA_REGISTER_SECTOR_COUNT(portBase),   0x55);
    outb(ATA_REGISTER_ADDR1(portBase),          0xAA);
    outb(ATA_REGISTER_SECTOR_COUNT(portBase),   0xAA);
    outb(ATA_REGISTER_ADDR1(portBase),          0x55);
    outb(ATA_REGISTER_SECTOR_COUNT(portBase),   0x55);
    outb(ATA_REGISTER_ADDR1(portBase),          0xAA);

    if (inb(ATA_REGISTER_SECTOR_COUNT(portBase)) != 0x55 || inb(ATA_REGISTER_ADDR1(portBase)) != 0xAA && inb(ATA_REGISTER_DEVICE(portBase)) != (deviceSelect ? ATA_DEVICE_DEVICE1 : ATA_DEVICE_DEVICE0)) {
        return RESULT_FAIL;
    }

    void* buffer = allocateBuffer(BUFFER_SIZE_512);

    if (__identifyATAPIdevice(channel, buffer) == RESULT_SUCCESS) {
        PacketDeviceIdentifyData* data = (PacketDeviceIdentifyData*)buffer;
        device->sectorNum = -1;
    } else if (__identifyATAdevice(channel, buffer) == RESULT_SUCCESS) {
        DeviceIdentifyData* data = (DeviceIdentifyData*)buffer;
        device->sectorNum = data->commandSetSupport.lba48Supported ? data->maxUserLBAfor48bitAddress : data->addressableSectorNum;
    } else {
        releaseBuffer(buffer, BUFFER_SIZE_512);
        return RESULT_FAIL;
    }

    device->deviceNumber = deviceSelect;

    releaseBuffer(buffer, BUFFER_SIZE_512);
    return RESULT_SUCCESS;
}

static Result __identifyATAdevice(ATAchannel* channel, void* buffer) {
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    ATAcommand command;
    memset(&command, 0, sizeof(ATAcommand));
    command.command = ATA_COMMAND_IDENTIFY_DEVICE;

    if (ATA_sendCommand(channel, &command) == RESULT_FAIL || ATA_PIOreadBlocks(portBase, 1, buffer) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __identifyATAPIdevice(ATAchannel* channel, void* buffer) {
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    ATAcommand command;
    memset(&command, 0, sizeof(ATAcommand));
    command.command = ATA_COMMAND_IDENTIFY_PACKET_DEVICE;

    if (ATA_sendCommand(channel, &command) == RESULT_FAIL || ATA_PIOreadBlocks(portBase, 1, buffer) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __ATAreadBlocks(BlockDevice* this, Index64 blockIndex, void* buffer, Size n) {
    ATAdevice* device = (ATAdevice*)this->handle;

    Index32 LBA28 = blockIndex;
    ATAcommand command = {
        .command = ATA_COMMAND_READ_SECTORS,
        .device = (device->channel->devices[0] == device ? ATA_DEVICE_DEVICE0 : ATA_DEVICE_DEVICE1) | ATA_DEVICE_LBA | EXTRACT_VAL(LBA28, 32, 24, 28),
        .feature = 0,
        .sectorCount = n,
        .addr1 = EXTRACT_VAL(LBA28, 32, 0, 8),
        .addr2 = EXTRACT_VAL(LBA28, 32, 8, 16),
        .addr3 = EXTRACT_VAL(LBA28, 32, 16, 24),
    };
    return ATA_PIOreadData(device, &command, buffer);
}

static Result __ATAwriteBlocks(BlockDevice* this, Index64 blockIndex, const void* buffer, Size n) {
    ATAdevice* device = (ATAdevice*)this->handle;

    Index32 LBA28 = blockIndex;
    ATAcommand command = {
        .command = ATA_COMMAND_WRITE_SECTORS,
        .device = (device->channel->devices[0] == device ? ATA_DEVICE_DEVICE0 : ATA_DEVICE_DEVICE1) | ATA_DEVICE_LBA | EXTRACT_VAL(LBA28, 32, 24, 28),
        .feature = 0,
        .sectorCount = n,
        .addr1 = EXTRACT_VAL(LBA28, 32, 0, 8),
        .addr2 = EXTRACT_VAL(LBA28, 32, 8, 16),
        .addr3 = EXTRACT_VAL(LBA28, 32, 16, 24),
    };
    return ATA_PIOwriteData(device, &command, buffer);
}