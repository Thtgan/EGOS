#if !defined(__DEVICES_ATA_CHANNEL_H)
#define __DEVICES_ATA_CHANNEL_H

typedef struct ATAchannel ATAchannel;

#include<devices/ata/ata.h>
#include<kit/types.h>

typedef struct ATAchannel {
    Uint16 portBase;
    ATAdevice* devices[2];
    Uint8 deviceSelect;
} ATAchannel;

OldResult ata_channel_reset(ATAchannel* channel);

bool ata_channel_selectDevice(ATAchannel* channel, Uint8 deviceSelect);

#endif // __DEVICES_ATA_CHANNEL_H
