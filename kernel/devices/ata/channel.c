#include<devices/ata/channel.h>

#include<devices/ata/ata.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>

Result ATA_resetChannel(ATAchannel* channel) {
    Uint16 portBase = channel->portBase;
    outb(ATA_REGISTER_CONTROL(portBase), ATA_CONTROL_SOFTWARE_RESET | ATA_CONTROL_NO_INTERRUPT);
    ATA_DELAY_400NS(portBase);
    outb(ATA_REGISTER_CONTROL(portBase), ATA_CONTROL_SOFTWARE_RESET);
    ATA_DELAY_400NS(portBase);

    if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
        return RESULT_FAIL;
    }

    if (channel->deviceSelect < 2) {
        ATA_selectDevice(channel, channel->deviceSelect);

        if (TEST_FLAGS(ATA_waitTillClear(portBase, ATA_STATUS_FLAG_BUSY), ATA_STATUS_FLAG_BUSY)) {
            return RESULT_FAIL;
        }
    }

    RESULT_SUCCESS;
}

bool ATA_selectDevice(ATAchannel* channel, Uint8 deviceSelect) {
    if (channel->deviceSelect == deviceSelect) {
        return false;
    }

    outb(ATA_REGISTER_DEVICE(channel->portBase), deviceSelect ? ATA_DEVICE_DEVICE1 : ATA_DEVICE_DEVICE0);
    ATA_DELAY_400NS(channel->portBase);

    channel->deviceSelect = deviceSelect;

    return true;
}