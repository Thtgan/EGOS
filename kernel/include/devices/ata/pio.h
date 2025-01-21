#if !defined(__DEVICES_ATA_PIO_H)
#define __DEVICES_ATA_PIO_H

#include<kit/types.h>
#include<devices/ata/ata.h>

void ata_pio_readData(ATAdevice* device, ATAcommand* command, void* buffer);

void ata_pio_writeData(ATAdevice* device, ATAcommand* command, const void* buffer);

void ata_pio_noData(ATAdevice* device, ATAcommand* command);

void ata_pio_readBlocks(Uint16 channelPortBase, Size n, void* buffer);

void ata_pio_writeBlocks(Uint16 channelPortBase, Size n, const void* buffer);

#endif // __DEVICES_ATA_PIO_H
