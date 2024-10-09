#if !defined(__DEVICES_ATA_ATA_H)
#define __DEVICES_ATA_ATA_H

typedef enum ATAdeviceType {
    ATA_DEVICE_TYPE_UNKNOWN,
    ATA_DEVICE_TYPE_PATA,
    ATA_DEVICE_TYPE_PATA_API,
    ATA_DEVICE_TYPE_SATA,
    ATA_DEVICE_TYPE_SATA_API
} ATAdeviceType;

typedef struct ATAdevice ATAdevice;
typedef struct ATAcommand ATAcommand;

#include<devices/ata/channel.h>
#include<devices/ata/identifyDevice.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>

#define ATA_SECTOR_SIZE                                     512

#define ATA_CANNNEL_BASE_1                                  0x01F0
#define ATA_CANNNEL_BASE_2                                  0x0170

#define ATA_REGISTER_DATA(__CHANNEL_BASE)                   (__CHANNEL_BASE + 0)
#define ATA_REGISTER_ERROR(__CHANNEL_BASE)                  (__CHANNEL_BASE + 1)
#define ATA_REGISTER_FEATURE(__CHANNEL_BASE)                (__CHANNEL_BASE + 1)
#define ATA_REGISTER_SECTOR_COUNT(__CHANNEL_BASE)           (__CHANNEL_BASE + 2)
#define ATA_REGISTER_ADDR1(__CHANNEL_BASE)                  (__CHANNEL_BASE + 3) //LBA28_0-7
#define ATA_REGISTER_ADDR2(__CHANNEL_BASE)                  (__CHANNEL_BASE + 4) //LBA28_8-15
#define ATA_REGISTER_ADDR3(__CHANNEL_BASE)                  (__CHANNEL_BASE + 5) //LBA28_16-23
#define ATA_REGISTER_DEVICE(__CHANNEL_BASE)                 (__CHANNEL_BASE + 6) //LBA28_24-27
#define ATA_REGISTER_STATUS(__CHANNEL_BASE)                 (__CHANNEL_BASE + 7) //Read only
#define ATA_REGISTER_COMMAND(__CHANNEL_BASE)                (__CHANNEL_BASE + 7)
#define ATA_REGISTER_CONTROL(__CHANNEL_BASE)                (__CHANNEL_BASE + 0x206)
#define ATA_REGISTER_ALT_STATUS(__CHANNEL_BASE)             (__CHANNEL_BASE + 0x206)

//No bit definitions for errors, they got different definition for EACH command, it's too much

#define ATA_CONTROL_NO_INTERRUPT                            FLAG8(1)
#define ATA_CONTROL_SOFTWARE_RESET                          FLAG8(2)

#define ATA_DEVICE_LBA                                      FLAG8(6)
#define ATA_DEVICE_DEVICE0                                  0xA0
#define ATA_DEVICE_DEVICE1                                  0xB0

#define ATA_STATUS_FLAG_ERROR                               FLAG8(0)
#define ATA_STATUS_FLAG_DATA_REQUIRE_SERVICE                FLAG8(3)
#define ATA_STATUS_FLAG_DEVICE_FAULT                        FLAG8(5)
#define ATA_STATUS_FLAG_STREAM_ERROR                        FLAG8(5)
#define ATA_STATUS_FLAG_READY                               FLAG8(6)
#define ATA_STATUS_FLAG_BUSY                                FLAG8(7)

#define ATA_DELAY_400NS(__BUS_PORT_BASE) {                  \
    Uint16 port = ATA_REGISTER_ALT_STATUS(__BUS_PORT_BASE); \
    inb(port); inb(port); inb(port); inb(port);             \
}

//Reference: ATA/ATAPI-7 specification

#define ATA_COMMAND_IDENTIFY_DEVICE                         0xEC
#define ATA_COMMAND_IDENTIFY_PACKET_DEVICE                  0xA1
#define ATA_COMMAND_READ_SECTORS                            0x20
#define ATA_COMMAND_WRITE_SECTORS                           0x30

typedef struct ATAdevice {
    char name[8];
    ATAdeviceType type;
    Uint8 deviceNumber;
    ATAchannel* channel;
    Size sectorNum;
} ATAdevice;

typedef struct ATAcommand {
    Uint8 command;
    Uint8 device;

    Uint8 feature;
    Uint8 sectorCount;
    Uint8 addr1;
    Uint8 addr2;
    Uint8 addr3;
} ATAcommand;

Result ata_initDevices();

Result ata_sendCommand(ATAchannel* channel, ATAcommand* command);

Flags8 ata_waitTillClear(Uint16 channelPortBase, Flags8 waitFlags);

Flags8 ata_waitTillSet(Uint16 channelPortBase, Flags8 waitFlags);

Result ata_waitForData(Uint16 channelPortBase);

#endif // __DEVICES_ATA_ATA_H
