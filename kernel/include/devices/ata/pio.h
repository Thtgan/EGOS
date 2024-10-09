#if !defined(__DEVICES_ATA_PIO_H)
#define __DEVICES_ATA_PIO_H

#include<kit/types.h>
#include<devices/ata/ata.h>

Result ata_pio_readData(ATAdevice* device, ATAcommand* command, void* buffer);

Result ata_pio_writeData(ATAdevice* device, ATAcommand* command, const void* buffer);

Result ata_pio_noData(ATAdevice* device, ATAcommand* command);

Result ata_pio_readBlocks(Uint16 channelPortBase, Size n, void* buffer);

Result ata_pio_writeBlocks(Uint16 channelPortBase, Size n, const void* buffer);

#endif // __DEVICES_ATA_PIO_H
