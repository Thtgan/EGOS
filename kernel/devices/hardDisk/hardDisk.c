#include<devices/hardDisk/hardDisk.h>

#include<devices/timer/timer.h>
#include<fs/blockDevice/blockDevice.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/macro.h>
#include<lib/blowup.h>
#include<memory/memory.h>
#include<memory/malloc.h>
#include<real/ports/HDC.h>
#include<real/simpleAsmLines.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>
#include<stdio.h>
#include<string.h>
#include<structs/singlyLinkedList.h>
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
    bool available, isMaster, isATA, isBoot;
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
 * @brief Translate CHS address to LBA address
 * 
 * @param chs CHS address
 * @param d Disk
 * @return LBA28_t LBA address
 */
static inline LBA28_t __CHS2LBA(const CHSAddress* chs, const Disk* d);

/**
 * @brief Translate LBA address to CHS address
 * 
 * @param chs CHS address to write in
 * @param lba LBA address
 * @param d Disk
 */
static inline void __LBA2CHS(CHSAddress* chs, LBA28_t lba, const Disk* d);

/**
 * @brief Wait channel till wait flags cleared, or retry times out
 * 
 * @param ch Channel to wait
 * @param waitFlags Flags to wait for clear
 * @return uint8_t Final status returned
 */
static uint8_t __waitChannel(const Channel* ch, uint8_t waitFlags);

/**
 * @brief Select device d
 * 
 * @param d Device to select
 */
static uint8_t __selectDevice(const Disk* d);

/**
 * @brief Check if the disk is an ATA device via device's signature, better call this after resetting the channel
 * 
 * @param d Disk to check
 * @return If the disk is an ATA device
 */
static bool __isATAdevice(Disk* d);

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

static inline LBA28_t __CHS2LBA(const CHSAddress* chs, const Disk* d) {
    const DeviceIdentifyData* params = d->parameters;
    return (((uint32_t)chs->cylinder) * params->headNum + chs->head) * params->sectorNumPerTrack + chs->sector - 1;
}

static inline void __LBA2CHS(CHSAddress* chs, LBA28_t lba, const Disk* d) {
    const DeviceIdentifyData* params = d->parameters;
    chs->cylinder = lba / (params->headNum * params->sectorNumPerTrack);
    chs->head = (lba / params->sectorNumPerTrack) % params->headNum;
    chs->sector = lba % params->sectorNumPerTrack + 1;
}

#define RETRY_TIME 65535

static uint8_t __waitChannel(const Channel* ch, uint8_t waitFlags) {
    uint16_t retry = RETRY_TIME;
    uint8_t status;
    do {
        status = inb(HDC_ALT_STATUS(ch->portBase));
    } while (TEST_FLAGS_CONTAIN(status, waitFlags) && retry-- != 0);
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

    uint8_t ret = inb(HDC_ALT_STATUS(portBase));
    return ret;
}

static bool __isATAdevice(Disk* d) {
    __selectDevice(d);

    uint16_t portBase = d->channel->portBase;

    uint8_t
        signature1 = inb(HDC_LBA_8_15(portBase)),
        signature2 = inb(HDC_LBA_16_23(portBase));

    return 
        (signature1 == 0x00 && signature2 == 0x00) ||
        (signature1 == 0x3C && signature2 == 0xC3);
}

static uint8_t __identifyDevice(Disk* d) {
    __selectDevice(d);

    Channel* c = d->channel;

    __postChannelCommand(c, HDC_COMMAND_IDENTIFY_DEVICE);

    while (c->waitingForInterrupt) {
        nop();
    }   //TODO: Replace this with semaphore implementation

    uint8_t status = __waitChannel(c, HDC_STATUS_BUSY);

    if (
        TEST_FLAGS_FAIL(status, HDC_STATUS_DATA_REQUIRE_SERVICE)    ||
        TEST_FLAGS(status, HDC_STATUS_ERROR)
    ) {
        return status;
    }

    d->available = true;

    insw(HDC_DATA(c->portBase), d->parameters, sizeof(DeviceIdentifyData) / sizeof(uint16_t));

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
    EOI();
}

ISR_FUNC_HEADER(__channel2Handler) {
    if (_channels[1].waitingForInterrupt) {
        _channels[1].waitingForInterrupt = false;
        inb(HDC_STATUS(_channels[1].portBase));
    } else {
        blowup("IRQ 15 not expected!");
    }
    EOI();
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
            
            memcpy(d->name, _nameTemplate, sizeof(_nameTemplate));  //TODO: replace this with sprintf
            d->name[2] = 'a' + (i << 1) + j;

            d->channel = &_channels[i];

            d->available = false;
            d->isMaster = (j == 0);
            d->isATA = __isATAdevice(d);

            if (!d->isATA) {
                printf("%s hard disk is not an ATA device, ignored.\n", d->name);
                continue;
            }

            uint8_t identifyStatus = __identifyDevice(d);

            if (!d->available) { //TODO: May fail here, might be caused by delay
                printf("%s hard disk not available, final status: %#02X.\n", d->name, identifyStatus);
                continue;
            }

            d->isBoot = __checkBootDisk(d);

            //d->freeSectorBegin = d->isBoot ? (BOOT_DISK_FREE_BEGIN / SECTOR_SIZE) : 0;
            d->freeSectorBegin = 0;

            __registerDiskBlockDevice(d);   //Register as the block device
        }
    }
}

static void __resetChannel(const Channel* c) {
    uint16_t portBase = c->portBase;
    outb(HDC_CONTROL(portBase), 0);
    outb(HDC_CONTROL(portBase), 4);
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

        uint8_t status = __waitChannel(c, HDC_STATUS_BUSY);

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
        uint8_t status = __waitChannel(c, HDC_STATUS_BUSY);

        outsw(HDC_DATA(c->portBase), buffer, SECTOR_SIZE / sizeof(uint16_t));

        while (c->waitingForInterrupt) {
            nop();
        }   //TODO: Replace this with semaphore implementation

        buffer += SECTOR_SIZE;
    }
}

//Function for block device, so no prototypes
//Read the block from disk, additional data is actually the disk struct
//TODO: According to collected info, lambda is possible in C, replace these with lambda
static void __readBlock(void* additionalData, block_index_t block, void* buffer, size_t n) {
    Disk* d = (Disk*)additionalData;
    __readSectors(d, d->freeSectorBegin + block, buffer, n);
}

//Write the block to disk, additional data is actually the disk struct
static void __writeBlock(void* additionalData, block_index_t block, const void* buffer, size_t n) {
    Disk* d = (Disk*)additionalData;
    __writeSectors(d, d->freeSectorBegin + block, buffer, n);
}

static void __registerDiskBlockDevice(Disk* d) {
    BlockDevice* device = createBlockDevice(d->name, d->parameters->addressableSectorNum - d->freeSectorBegin, BLOCK_DEVICE_TYPE_DISK);

    device->additionalData = d; //Disk block device's additional data is itself

    device->readBlocks = __readBlock;
    device->writeBlocks = __writeBlock;
    
    registerBlockDevice(device);
}

static bool __checkBootDisk(Disk* d) {
    uint16_t* MBR = malloc(SECTOR_SIZE);    //TODO: Try to replace with a reserved buffer?
    __readSectors(d, 0, MBR, 1);

    bool ret = (MBR[219] == SYSTEM_INFO_MAGIC16) && (MBR[255] == 0xAA55);

    free(MBR);

    return ret;
}