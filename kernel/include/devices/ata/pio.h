#if !defined(__PIO_H)
#define __PIO_H

#include<kit/types.h>

#include<devices/ata/ata.h>

Result ATA_PIOreadData(ATAdevice* device, ATAcommand* command, void* buffer);

Result ATA_PIOwriteData(ATAdevice* device, ATAcommand* command, const void* buffer);

Result ATA_PIOnoData(ATAdevice* device, ATAcommand* command);

Result ATA_PIOreadBlocks(Uint16 channelPortBase, Size n, void* buffer);

Result ATA_PIOwriteBlocks(Uint16 channelPortBase, Size n, const void* buffer);

#endif // __PIO_H
