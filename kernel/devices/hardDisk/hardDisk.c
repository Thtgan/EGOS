#include<devices/hardDisk/hardDisk.h>

#include<devices/timer/timer.h>
#include<kit/bit.h>
#include<memory/memory.h>
#include<real/ports/HDC.h>
#include<real/simpleAsmLines.h>
#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<system/deviceIdentify.h>

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
    bool present, available, isMaster, isATA;
    Channel* channel;
    DeviceIdentifyData* parameters; //Data inside is meanful iff available is true
} Disk;

struct __Channel {
    uint16_t portBase;
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
 * @brief Read identify data of the device
 * 
 * @param d Disk device to read
 * @return uint8_t Finally returned status 
 */
static uint8_t __identifyDevice(Disk* d);

/**
 * @brief Reset the channel, this will make the registers load the device's signature
 * 
 * @param c Channel to reset
 */
static void __resetChannel(Channel* c);

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
        status = inb(HDC_STATUS(ch->portBase));
    } while (TEST_FLAGS_CONTAIN(status, waitFlags) && retry-- != 0);
    return status;
}

static uint8_t __selectDevice(const Disk* d) {
    uint16_t portBase = d->channel->portBase;
    uint8_t select = HDC_DEVICE_SELECT_BASE;
    if (!d->isMaster) {
        SET_FLAG_BACK(select, HDC_DEVICE_SELECT_SLAVE);
    }
    outb(HDC_DEVICE_SELECT(portBase), select);

    sleep(NANOSECOND, 400);

    uint8_t ret = inb(HDC_ALT_STATUS(portBase));
    return ret;
}

static bool __isATAdevice(Disk* d) {
    __selectDevice(d);

    uint16_t portBase = d->channel->portBase;

    uint8_t
        signature1 = inb(HDC_LBA_MID(portBase)),
        signature2 = inb(HDC_LBA_HIGH(portBase));

    printf("%s signature1: %#02X, signature2: %#02X\n", d->name, signature1, signature2);
    return 
        (signature1 == 0x00 && signature2 == 0x00) ||
        (signature1 == 0x3C && signature2 == 0xC3);
}

static uint8_t __identifyDevice(Disk* d) {
    __waitChannel(d->channel, HDC_STATUS_BUSY | HDC_STATUS_DATA_REQUIRE_SERVICE);
    __selectDevice(d);
    __waitChannel(d->channel, HDC_STATUS_BUSY | HDC_STATUS_DATA_REQUIRE_SERVICE);

    uint16_t portBase = d->channel->portBase;

    outw(HDC_COMMAND(portBase), HDC_COMMAND_IDENTIFY_DEVICE);

    sleep(SECOND, 1);   //TODO: Replace this with semaphore control

    uint8_t status = __waitChannel(d->channel, HDC_STATUS_BUSY);

    d->available = false;
    if (
        TEST_FLAGS_FAIL(status, HDC_STATUS_DATA_REQUIRE_SERVICE)    ||
        TEST_FLAGS(status, HDC_STATUS_ERROR)
    ) {
        return status;
    }

    d->available = true;

    insw(HDC_DATA(portBase), d->parameters, sizeof(DeviceIdentifyData) / sizeof(uint16_t));

    return status;
}

const uint16_t _channelPortBases[2] = {
    HDC_CANNNEL_1,
    HDC_CANNNEL_2
};

Channel _channels[2];
static char* nameTemplate = "hd_";

void initHardDisk() {
    for (int i = 0; i < 2; ++i) {
        _channels[i].portBase = _channelPortBases[i];

        __resetChannel(&_channels[i]);

        for (int j = 0; j < 2; ++j) {
            Disk* d = &_channels[i].disks[j];
            
            memcpy(d->name, nameTemplate, sizeof(nameTemplate));
            d->name[2] = 'a' + (i << 1) + j;

            d->channel = &_channels[i];
            d->isMaster = (j == 0);
            d->isATA = __isATAdevice(d);

            if (!d->isATA) {
                printf("%s hard disk is not an ATA device, ignored.\n", d->name);
                continue;
            }

            uint8_t identifyStatus = __identifyDevice(d);

            if (!d->available) {
                printf("%s hard disk not available, final status: %#02X.\n", d->name, identifyStatus);
                continue;
            }

            printf("%s available sector num: %u\n", d->name, d->parameters->addressableSectorNum);
        }
    }
}

static void __resetChannel(Channel* c) {
    uint16_t portBase = c->portBase;

    outb(HDC_CONTROL(portBase), 0);
    outb(HDC_CONTROL(portBase), 4);
    outb(HDC_CONTROL(portBase), 0);
}