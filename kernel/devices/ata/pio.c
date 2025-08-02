#include<devices/ata/pio.h>

#include<devices/ata/ata.h>
#include<real/simpleAsmLines.h>
#include<error.h>

void ata_pio_readData(ATAdevice* device, ATAcommand* command, void* buffer) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    ata_channel_selectDevice(channel, device != channel->devices[0]);
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    ata_sendCommand(channel, command);
    ERROR_GOTO_IF_ERROR(0);

    ata_pio_readBlocks(portBase, command->sectorCount, buffer);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void ata_pio_writeData(ATAdevice* device, ATAcommand* command, const void* buffer) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    ata_sendCommand(channel, command);
    ERROR_GOTO_IF_ERROR(0);

    ata_pio_writeBlocks(portBase, command->sectorCount, buffer);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void ata_pio_noData(ATAdevice* device, ATAcommand* command) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        ERROR_THROW(ERROR_ID_IO_FAILED, 0);
    }

    ata_sendCommand(channel, command);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void ata_pio_readBlocks(Uint16 channelPortBase, Size n, void* buffer) {
    while (n--) {
        ata_waitForData(channelPortBase);
        ERROR_GOTO_IF_ERROR(0);

        insw(ATA_REGISTER_DATA(channelPortBase), buffer, ATA_SECTOR_SIZE / sizeof(Uint16));

        buffer += ATA_SECTOR_SIZE;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void ata_pio_writeBlocks(Uint16 channelPortBase, Size n, const void* buffer) {
    while (n--) {
        ata_waitForData(channelPortBase);
        ERROR_GOTO_IF_ERROR(0);

        outsw(ATA_REGISTER_DATA(channelPortBase), buffer, ATA_SECTOR_SIZE / sizeof(Uint16));

        buffer += ATA_SECTOR_SIZE;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}
