#if !defined(__DEVICES_ATA_CHANNEL_H)
#define __DEVICES_ATA_CHANNEL_H

#include<devices/ata/ata.h>
#include<kit/types.h>

Result ata_channel_reset(ATAchannel* channel);

bool ata_channel_selectDevice(ATAchannel* channel, Uint8 deviceSelect);

#endif // __DEVICES_ATA_CHANNEL_H
