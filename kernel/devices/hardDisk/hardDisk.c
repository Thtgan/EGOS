#include<devices/hardDisk/hardDisk.h>

#include<debug.h>
#include<devices/block/blockDevice.h>
#include<devices/block/blockDeviceTypes.h>
#include<devices/terminal/terminalSwitch.h>
#include<devices/timer/timer.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/macro.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/buffer.h>
#include<print.h>
#include<real/ports/HDC.h>
#include<real/simpleAsmLines.h>
#include<returnValue.h>
#include<string.h>
#include<system/address.h>
#include<system/deviceIdentify.h>
#include<system/systemInfo.h>

__attribute__((aligned(sizeof(DeviceIdentifyData))))
DeviceIdentifyData deviceIdentifies[4];

typedef uint32_t LBA28_t;

typedef struct {
    uint16_t cylinder;
    uint8_t head;
    uint8_t sector;
} CHSAddress;

struct __Channel;

typedef struct __Channel Channel;

typedef struct {
    char name[8];
    bool available, isMaster, isBoot;
    Channel* channel;
    LBA28_t freeSectorBegin;
    DeviceIdentifyData* parameters; //Data inside is meanful iff available is true
} Disk;

struct __Channel {
    uint16_t portBase;
    bool waitingForInterrupt;
    Disk disks[2];
};

/**
 * @brief Wait channel till wait flags cleared, or retry times out
 * 
 * @param ch Channel to wait
 * @param waitFlags Flags to wait for clear
 * @return uint8_t Final status returned
 */
static uint8_t __waitChannelClear(const Channel* ch, uint8_t waitFlags);

/**
 * @brief Wait channel till wait flags set, or retry times out
 * 
 * @param ch Channel to wait
 * @param waitFlags Flags to wait for set
 * @return uint8_t Final status returned
 */
static uint8_t __waitChannelSet(const Channel* ch, uint8_t waitFlags);

/**
 * @brief Select device d
 * 
 * @param d Device to select
 */
static uint8_t __selectDevice(const Disk* d);

/**
 * @brief Read identify data of the device, and check device's availibility
 * 
 * @param d Disk device to read
 * @return uint8_t Finally returned status 
 */
static uint8_t __identifyDevice(Disk* d);

/**
 * @brief Reset the channel, this will make the registers load the device's signature and fire the controller interrupt
 * 
 * @param c Channel to reset
 */
static void __resetChannel(const Channel* c);

/**
 * @brief Post command to the device, with notify the channel waiting for the interrupt
 * 
 * @param c Channel
 * @param command Command to post
 */
static void __postChannelCommand(Channel* c, uint8_t command);

/**
 * @brief Select a sector on the disk through the lba
 * 
 * @param d Disk contains the sector to select
 * @param lba LBA of the sector to select
 */
static void __selectSector(const Disk* d, LBA28_t lba);

/**
 * @brief Read continued sector(s) in selected disk to buffer, first sector located by lba
 * 
 * @param d Disk to read
 * @param lba LBA to the first sector
 * @param buffer Buffer to read data to
 * @param n Num of sectors to read
 */
static void __readSectors(Disk* d, LBA28_t lba, void* buffer, uint8_t n);

/**
 * @brief Write continued sector(s) in selected disk with data from buffer, first sector located by lba
 * 
 * @param d Disk to write
 * @param lba LBA to the firstsector
 * @param buffer Buffer contains data to write
 * @param Num of sectors to write
 */
static void __writeSectors(Disk* d, LBA28_t lba, const void* buffer, uint8_t n);

/**
 * @brief Register the disk as a block device
 * 
 * @param d Disk to register
 */
static void __registerDiskBlockDevice(Disk* d);

/**
 * @brief Check if the system is booted from this disk, hoping there is no two disk with same signature, call after reading is ready
 * 
 * @param d Disk to check
 * @return If the system is booted from this disk
 */
static bool __checkBootDisk(Disk* d);

static int __readBlocks(BlockDevice* this, block_index_t blockIndex, void* buffer, size_t n);

static int __writeBlocks(BlockDevice* this, block_index_t blockIndex, const void* buffer, size_t n);

static BlockDeviceOperation _operations = {
    .readBlocks = __readBlocks,
    .writeBlocks = __writeBlocks
};

#define RETRY_TIME 65535

static uint8_t __waitChannelClear(const Channel* ch, uint8_t waitFlags) {
    uint16_t retry = RETRY_TIME;
    uint8_t status;
    do {
        status = inb(HDC_ALT_STATUS(ch->portBase));
        sleep(MICROSECOND, 1);
    } while (TEST_FLAGS_CONTAIN(status, waitFlags) && retry-- != 0);
    return status;
}

static uint8_t __waitChannelSet(const Channel* ch, uint8_t waitFlags) {
    uint16_t retry = RETRY_TIME;
    uint8_t status;
    do {
        status = inb(HDC_ALT_STATUS(ch->portBase));
        sleep(MICROSECOND, 1);
    } while (TEST_FLAGS_NONE(status, waitFlags) && retry-- != 0);
    return status;
}

static uint8_t __selectDevice(const Disk* d) {
    uint16_t portBase = d->channel->portBase;
    uint8_t select = HDC_DEVICE_SELECT_BASE;
    if (!d->isMaster) {
        SET_FLAG_BACK(select, HDC_DEVICE_SELECT_SLAVE);
    }
    outb(HDC_LBA_24_27_DEVICE_SELECT(portBase), select);

    sleep(NANOSECOND, 400);

    return inb(HDC_ALT_STATUS(portBase));
}

static uint8_t __identifyDevice(Disk* d) {
    d->available = false;
    __selectDevice(d);

    Channel* c = d->channel;

    outb(HDC_SECTOR_COUNT(c->portBase), 0);
    outb(HDC_LBA_0_7(c->portBase), 0);
    outb(HDC_LBA_8_15(c->portBase), 0);
    outb(HDC_LBA_16_23(c->portBase), 0);

    __postChannelCommand(c, HDC_COMMAND_IDENTIFY_DEVICE);

    uint8_t status = inb(HDC_STATUS(c->portBase));
    if (status == 0) {
        return 0;
    }

    status = __waitChannelClear(c, HDC_STATUS_BUSY);
    if (!(inb(HDC_LBA_8_15(c->portBase)) == 0 && inb(HDC_LBA_16_23(c->portBase)) == 0)) {
        d->parameters->generalConfiguration.isNotATA = true;
        return status;
    }

    __waitChannelSet(c, HDC_STATUS_ERROR | HDC_STATUS_DATA_REQUIRE_SERVICE);

    if (!TEST_FLAGS(status, HDC_STATUS_ERROR)) {
        insw(HDC_DATA(c->portBase), d->parameters, sizeof(DeviceIdentifyData) / sizeof(uint16_t));
    }
    d->available = true;

    return status;
}

const uint16_t _channelPortBases[2] = {
    HDC_CANNNEL_1,
    HDC_CANNNEL_2
};

Channel _channels[2];
static char* _nameTemplate = "hd_";

ISR_FUNC_HEADER(__channel1Handler) {
    if (_channels[0].waitingForInterrupt) {
        _channels[0].waitingForInterrupt = false;
        inb(HDC_STATUS(_channels[0].portBase));
    } else {
        blowup("IRQ 14 not expected!");
    }
}

ISR_FUNC_HEADER(__channel2Handler) {
    if (_channels[1].waitingForInterrupt) {
        _channels[1].waitingForInterrupt = false;
        inb(HDC_STATUS(_channels[1].portBase));
    } else {
        blowup("IRQ 15 not expected!");
    }
}

void initHardDisk() {
    registerISR(0x2E, __channel1Handler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
    registerISR(0x2F, __channel2Handler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
    for (int i = 0; i < 2; ++i) {
        _channels[i].portBase = _channelPortBases[i];
        _channels[i].waitingForInterrupt = true;

        __resetChannel(&_channels[i]);

        for (int j = 0; j < 2; ++j) {
            Disk* d = &_channels[i].disks[j];
            sprintf(d->name, "hd%c", 'a' + (i << 1) + j);

            d->channel = &_channels[i];

            d->available = false;
            d->isMaster = (j == 0);

            uint8_t identifyStatus = __identifyDevice(d);
            if (d->parameters->generalConfiguration.isNotATA) {
                printf(TERMINAL_LEVEL_DEBUG, "%s hard disk is not an ATA device, ignored.\n", d->name);
                continue;
            }

            if (!d->available) { //TODO: May fail here, might be caused by delay
                printf(TERMINAL_LEVEL_DEBUG, "%s hard disk not available, final status: %#02X.\n", d->name, identifyStatus);
                continue;
            }

            d->isBoot = __checkBootDisk(d);

            d->freeSectorBegin = d->isBoot ? (BOOT_DISK_FREE_BEGIN / SECTOR_SIZE) : 0;

            __registerDiskBlockDevice(d);   //Register as the block device
        }
    }
}

static void __resetChannel(const Channel* c) {
    uint16_t portBase = c->portBase;
    outb(HDC_CONTROL(portBase), 0);
    outb(HDC_CONTROL(portBase), 4); //Disk reset bit
    outb(HDC_CONTROL(portBase), 0);
}

static void __postChannelCommand(Channel* c, uint8_t command) {
    c->waitingForInterrupt = true;
    outb(HDC_COMMAND(c->portBase), command);
}

static void __selectSector(const Disk* d, LBA28_t lba) {
    __selectDevice(d);

    uint16_t portBase = d->channel->portBase;
    outb(HDC_LBA_0_7(portBase), EXTRACT_VAL(lba, 32, 0, 8));
    outb(HDC_LBA_8_15(portBase), EXTRACT_VAL(lba, 32, 8, 16));
    outb(HDC_LBA_16_23(portBase), EXTRACT_VAL(lba, 32, 16, 24));
    outb(
        HDC_LBA_24_27_DEVICE_SELECT(portBase), 
        HDC_DEVICE_SELECT_BASE                      |
        HDC_SECTOR_SELECT_LBA                       |
        (d->isMaster ? 0 : HDC_DEVICE_SELECT_SLAVE) |
        EXTRACT_VAL(lba, 32, 24, 28)
    );
}

static void __readSectors(Disk* d, LBA28_t lba, void* buffer, uint8_t n) {
    Channel* c = d->channel;

    __selectSector(d, lba);
    outb(HDC_SECTOR_COUNT(c->portBase), n);

    __postChannelCommand(c, HDC_COMMAND_READ_SECTORS);

    while (n--) {
        c->waitingForInterrupt = true;
        while (c->waitingForInterrupt) {
            nop();
        }   //TODO: Replace this with semaphore implementation

        __waitChannelClear(c, HDC_STATUS_BUSY);

        insw(HDC_DATA(c->portBase), buffer, SECTOR_SIZE / sizeof(uint16_t));
        buffer += SECTOR_SIZE;
    }
}

static void __writeSectors(Disk* d, LBA28_t lba, const void* buffer, uint8_t n) {
    Channel* c = d->channel;

    __selectSector(d, lba);
    outb(HDC_SECTOR_COUNT(c->portBase), n);

    __postChannelCommand(c, HDC_COMMAND_WRITE_SECTORS);

    while (n--) {
        c->waitingForInterrupt = true;
        __waitChannelClear(c, HDC_STATUS_BUSY);

        outsw(HDC_DATA(c->portBase), buffer, SECTOR_SIZE / sizeof(uint16_t));

        while (c->waitingForInterrupt) {
            nop();
        }   //TODO: Replace this with semaphore implementation

        buffer += SECTOR_SIZE;
    }
}

static void __registerDiskBlockDevice(Disk* d) {
    BlockDevice* device = createBlockDevice(d->name, HDD, d->parameters->addressableSectorNum - d->freeSectorBegin, &_operations, (Object)d);
    registerBlockDevice(device);
}

static bool __checkBootDisk(Disk* d) {
    uint16_t* MBR = allocateBuffer(BUFFER_SIZE_512);
    __readSectors(d, 0, MBR, 1);

    bool ret = (MBR[219] == SYSTEM_INFO_MAGIC16) && (MBR[255] == 0xAA55);

    releaseBuffer(MBR, BUFFER_SIZE_512);

    return ret;
}

static ReturnValue __readBlocks(BlockDevice* this, block_index_t blockIndex, void* buffer, size_t n) {
    Disk* d = (Disk*)this->additionalData;
    __readSectors(d, d->freeSectorBegin + blockIndex, buffer, n);
    return RETURN_VALUE_RETURN_NORMALLY;
}

static ReturnValue __writeBlocks(BlockDevice* this, block_index_t blockIndex, const void* buffer, size_t n) {
    Disk* d = (Disk*)this->additionalData;
    __writeSectors(d, d->freeSectorBegin + blockIndex, buffer, n);
    return RETURN_VALUE_RETURN_NORMALLY;
}