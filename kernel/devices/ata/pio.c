#include<devices/ata/pio.h>

#include<devices/ata/ata.h>
#include<real/simpleAsmLines.h>

Result ata_pio_readData(ATAdevice* device, ATAcommand* command, void* buffer) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    if (ata_sendCommand(channel, command) == RESULT_FAIL || ata_pio_readBlocks(portBase, command->sectorCount, buffer) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result ata_pio_writeData(ATAdevice* device, ATAcommand* command, const void* buffer) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    if (ata_sendCommand(channel, command) == RESULT_FAIL || ata_pio_writeBlocks(portBase, command->sectorCount, buffer) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result ata_pio_noData(ATAdevice* device, ATAcommand* command) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ata_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    if (ata_sendCommand(channel, command) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result ata_pio_readBlocks(Uint16 channelPortBase, Size n, void* buffer) {
    while (n--) {
        if (ata_waitForData(channelPortBase) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        insw(ATA_REGISTER_DATA(channelPortBase), buffer, ATA_SECTOR_SIZE / sizeof(Uint16));

        buffer += ATA_SECTOR_SIZE;
    }

    return RESULT_SUCCESS;
}

Result ata_pio_writeBlocks(Uint16 channelPortBase, Size n, const void* buffer) {
    while (n--) {
        if (ata_waitForData(channelPortBase) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        outsw(ATA_REGISTER_DATA(channelPortBase), buffer, ATA_SECTOR_SIZE / sizeof(Uint16));

        buffer += ATA_SECTOR_SIZE;
    }

    return RESULT_SUCCESS;
}
