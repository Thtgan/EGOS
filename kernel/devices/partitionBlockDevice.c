#include<devices/partitionBlockDevice.h>

#include<devices/blockDevice.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/mm.h>
#include<error.h>
#include<print.h>

typedef struct __MBRpartitionEntry {
    Uint8   driveArttribute;
    Uint8   beginHead;
    Uint8   beginSector     : 6;
    Uint16  beginCylinder   : 10;
    Uint8   systemID;
    Uint8   endHead;
    Uint8   endSector       : 6;
    Uint16  endCylinder     : 10;
    Uint32  sectorBegin;
    Uint32  sectorNum;
} __attribute__((packed)) __MBRpartitionEntry;

static void __partitionBlockDevice_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN);
static void __partitionBlockDevice_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN);
static void __partitionBlockDevice_flush(Device* device);

static DeviceOperations _partitionBlockDeviceOperations = {
    .readUnits  = __partitionBlockDevice_readUnits,
    .writeUnits = __partitionBlockDevice_writeUnits,
    .flush      = __partitionBlockDevice_flush
};

BlockDevice* blockDevice_bootFromDevice;    //TODO: Ugly, figure out a method to know which device we are booting from

void partitionBlockDevice_probePartitions(BlockDevice* parentDevice) {
    void* buffer = mm_allocate(BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    blockDevice_readBlocks(parentDevice, 0, buffer, 1);
    ERROR_GOTO_IF_ERROR(0);

    Device* device = &parentDevice->device;
    char nameBuffer[DEVICE_NAME_MAX_LENGTH + 1];
    PartitionBlockDevice* partitioBlockDevice = NULL;
    if (PTR_TO_VALUE(16, buffer + 0x1FE) != 0xAA55) {
        mm_free(buffer);
        return;
    }

    for (int i = 0; i < 4; ++i) {
        __MBRpartitionEntry* entry = (__MBRpartitionEntry*)(buffer + 0x1BE + i * sizeof(__MBRpartitionEntry));
        if (entry->systemID == 0) {
            continue;
        }

        print_snprintf(nameBuffer, sizeof(nameBuffer) - 1, "%s-p%d", device->name, i);

        MajorDeviceID major = DEVICE_MAJOR_FROM_ID(device->id);
        MinorDeviceID minor = device_allocMinor(major);
        if (minor == DEVICE_INVALID_ID) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        BlockDeviceInitArgs args = {
            .deviceInitArgs     = (DeviceInitArgs) {
                .id             = DEVICE_BUILD_ID(major, minor),
                .name           = nameBuffer,
                .parent         = device,
                .granularity    = device->granularity,
                .capacity       = entry->sectorNum,
                .flags          = CLEAR_FLAG(device->flags, DEVICE_FLAGS_BUFFERED),
                .operations     = &_partitionBlockDeviceOperations
            },
        };

        partitioBlockDevice = mm_allocate(sizeof(PartitionBlockDevice));
        if (partitioBlockDevice == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        blockDevice_initStruct(&partitioBlockDevice->blockDevice, &args);
        ERROR_GOTO_IF_ERROR(0);
        partitioBlockDevice->parentBegin = entry->sectorBegin;

        device_registerDevice(&partitioBlockDevice->blockDevice.device);
        ERROR_GOTO_IF_ERROR(0);
        
        if (blockDevice_bootFromDevice == NULL) {
            blockDevice_bootFromDevice = &partitioBlockDevice->blockDevice;
        }
        partitioBlockDevice = NULL;
    }

    mm_free(buffer);
    return;
    ERROR_FINAL_BEGIN(0);
    if (partitioBlockDevice != NULL) {
        mm_free(partitioBlockDevice);
    }

    if (buffer != NULL) {
        mm_free(buffer);
    }
}

static void __partitionBlockDevice_readUnits(Device* device, Index64 unitIndex, void* buffer, Size unitN) {
    PartitionBlockDevice* partitioBlockDevice = HOST_POINTER(device, PartitionBlockDevice, blockDevice.device);
    blockDevice_readBlocks(HOST_POINTER(device->parent, BlockDevice, device), partitioBlockDevice->parentBegin + unitIndex, buffer, unitN);
}

static void __partitionBlockDevice_writeUnits(Device* device, Index64 unitIndex, const void* buffer, Size unitN) {
    PartitionBlockDevice* partitioBlockDevice = HOST_POINTER(device, PartitionBlockDevice, blockDevice.device);
    blockDevice_writeBlocks(HOST_POINTER(device->parent, BlockDevice, device), partitioBlockDevice->parentBegin + unitIndex, buffer, unitN);
}

static void __partitionBlockDevice_flush(Device* device) {
    blockDevice_flush(HOST_POINTER(device->parent, BlockDevice, device));
}
