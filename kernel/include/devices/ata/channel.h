#if !defined(__CHANNEL_H)
#define __CHANNEL_H

#include<devices/ata/ata.h>
#include<kit/types.h>

Result ATA_resetChannel(ATAchannel* channel);

bool ATA_selectDevice(ATAchannel* channel, Uint8 deviceSelect);

#endif // __CHANNEL_H
