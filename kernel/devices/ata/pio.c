#include<devices/ata/pio.h>

#include<devices/ata/ata.h>
#include<real/simpleAsmLines.h>

Result ATA_PIOreadData(ATAdevice* device, ATAcommand* command, void* buffer) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    if (ATA_sendCommand(channel, command) == RESULT_FAIL || ATA_PIOreadBlocks(portBase, 1, buffer) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result ATA_PIOwriteData(ATAdevice* device, ATAcommand* command, const void* buffer) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    if (ATA_sendCommand(channel, command) == RESULT_FAIL || ATA_PIOwriteBlocks(portBase, 1, buffer) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result ATA_PIOnoData(ATAdevice* device, ATAcommand* command) {
    ATAchannel* channel = device->channel;
    Uint16 portBase = channel->portBase;
    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    if (ATA_sendCommand(channel, command) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result ATA_PIOreadBlocks(Uint16 channelPortBase, Size n, void* buffer) {
    while (n--) {
        if (ATA_waitForData(channelPortBase) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        insw(ATA_REGISTER_DATA(channelPortBase), buffer, ATA_SECTOR_SIZE / sizeof(Uint16));

        buffer += ATA_SECTOR_SIZE;
    }

    return RESULT_SUCCESS;
}

Result ATA_PIOwriteBlocks(Uint16 channelPortBase, Size n, const void* buffer) {
    while (n--) {
        if (ATA_waitForData(channelPortBase) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        outsw(ATA_REGISTER_DATA(channelPortBase), buffer, ATA_SECTOR_SIZE / sizeof(Uint16));

        buffer += ATA_SECTOR_SIZE;
    }

    return RESULT_SUCCESS;
}
