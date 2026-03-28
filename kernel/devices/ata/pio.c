#include<devices/ata/pio.h>

#include<devices/ata/ata.h>
#include<real/simpleAsmLines.h>
#include<lib/errorPosix.h>

void ata_pio_readData(ATAdevice* device, ATAcommand* command, void* buffer) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    ata_channel_selectDevice(channel, device != channel->devices[0]);
    ERROR_THROW_NEW_IF(TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY), ERROR_IO_FAILED, error_out);

    ata_sendCommand(channel, command);
    CHECK_ERROR(error_out);

    ata_pio_readBlocks(portBase, command->sectorCount, buffer);
    CHECK_ERROR(error_out);

    return;
error_out:
}

void ata_pio_writeData(ATAdevice* device, ATAcommand* command, const void* buffer) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    ERROR_THROW_NEW_IF(TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY), ERROR_IO_FAILED, error_out);

    ata_sendCommand(channel, command);
    CHECK_ERROR(error_out);

    ata_pio_writeBlocks(portBase, command->sectorCount, buffer);
    CHECK_ERROR(error_out);

    return;
error_out:
}

void ata_pio_noData(ATAdevice* device, ATAcommand* command) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    ERROR_THROW_NEW_IF(TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY), ERROR_IO_FAILED, error_out);

    ata_sendCommand(channel, command);
    CHECK_ERROR(error_out);

    return;
error_out:
}

void ata_pio_readBlocks(Uint16 channelPortBase, Size n, void* buffer) {
    while (n--) {
        ata_waitForData(channelPortBase);
        CHECK_ERROR(error_out);

        insw(ATA_REGISTER_DATA(channelPortBase), buffer, ATA_SECTOR_SIZE / sizeof(Uint16));

        buffer += ATA_SECTOR_SIZE;
    }

    return;
error_out:
}

void ata_pio_writeBlocks(Uint16 channelPortBase, Size n, const void* buffer) {
    while (n--) {
        ata_waitForData(channelPortBase);
        CHECK_ERROR(error_out);

        outsw(ATA_REGISTER_DATA(channelPortBase), buffer, ATA_SECTOR_SIZE / sizeof(Uint16));

        buffer += ATA_SECTOR_SIZE;
    }

    return;
error_out:
}
