#include<devices/block/blockDevice.h>

#include<devices/block/blockBuffer.h>
#include<devices/device.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<print.h>
#include<cstring.h>
#include<structs/hashTable.h>
#include<structs/singlyLinkedList.h>
#include<error.h>

static void __blockDevice_doProbePartitions(BlockDevice* device, void* buffer);

static void __blockDevice_partition_read(Device* device, Index64 index, void* buffer, Size n);
static void __blockDevice_partition_write(Device* device, Index64 index, const void* buffer, Size n);
static void __blockDevice_partition_flush(Device* device);

static DeviceOperations _blockDevice_partition_deviceOperations = {
    .read   = __blockDevice_partition_read,
    .write  = __blockDevice_partition_write,
    .flush  = __blockDevice_partition_flush
};

void blockDevice_initStruct(BlockDevice* blockDevice, BlockDeviceInitArgs* args) {
    if (args->deviceInitArgs.granularity == 0) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    Device* device = &blockDevice->device;
    device_initStruct(device, &args->deviceInitArgs);

    BlockBuffer* blockBuffer = NULL;
    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        blockBuffer = memory_allocate(sizeof(BlockBuffer));
        if (blockBuffer == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        blockBuffer_initStruct(blockBuffer, BLOCK_BUFFER_DEFAULT_HASH_SIZE, BLOCK_BUFFER_DEFAULT_MAX_BLOCK_NUM, device->granularity);
        ERROR_GOTO_IF_ERROR(1);

        blockDevice->blockBuffer = blockBuffer;
    }

    return;
    ERROR_FINAL_BEGIN(0);
    return;

    ERROR_FINAL_BEGIN(1);
    if (blockBuffer != NULL) {
        memory_free(blockBuffer);
    }
    ERROR_GOTO(0);
}

void blockDevice_readBlocks(BlockDevice* blockDevice, Index64 blockIndex, void* buffer, Size n) {
    Device* device = &blockDevice->device;
    if (device->operations->read == NULL) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);   //TODO: Move this to raw call function?
    }

    if (blockIndex >= device->capacity) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            BlockBufferBlock* block = blockBuffer_pop(blockDevice->blockBuffer, index);
            if (block == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

            if (TEST_FLAGS_FAIL(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_PRESENT)) {
                device_rawRead(device, index, block->data, 1);  //TODO: May be this happens too frequently?
                ERROR_GOTO_IF_ERROR(0);
            }

            memory_memcpy(buffer + ((Index64)i * POWER_2(device->granularity)), block->data, POWER_2(device->granularity));

            blockBuffer_push(blockDevice->blockBuffer, index, block);
            ERROR_GOTO_IF_ERROR(0);
        }

        return;
    }

    device_rawRead(device, blockIndex, buffer, n);
    ERROR_GOTO_IF_ERROR(0);
    return;
    ERROR_FINAL_BEGIN(0);
} 

void blockDevice_writeBlocks(BlockDevice* blockDevice, Index64 blockIndex, const void* buffer, Size n) {
    Device* device = &blockDevice->device;
    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_READONLY)) {
        ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
    }

    if (device->operations->write == NULL) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);   //TODO: Move this to raw call function?
    }

    if (blockIndex >= device->capacity) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        for (int i = 0; i < n; ++i) {
            Index64 index = blockIndex + i;
            BlockBufferBlock* block = blockBuffer_pop(blockDevice->blockBuffer, index);
            if (block == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

            if (index != block->blockIndex && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                device_rawWrite(device, block->blockIndex, block->data, 1); //TODO: May be this happens too frequently?
                ERROR_GOTO_IF_ERROR(0);
            }

            memory_memcpy(block->data, buffer + ((Index64)i * POWER_2(device->granularity)), POWER_2(device->granularity));
            SET_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);

            blockBuffer_push(blockDevice->blockBuffer, index, block);
            ERROR_GOTO_IF_ERROR(0);
        }

        return;
    }

    device_rawWrite(device, blockIndex, buffer, n);
    ERROR_GOTO_IF_ERROR(0);
    return;
    ERROR_FINAL_BEGIN(0);
}

void blockDevice_flush(BlockDevice* blockDevice) {
    Device* device = &blockDevice->device;
    if (device->operations->flush == NULL) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);   //TODO: Move this to raw call function?
    }

    if (TEST_FLAGS(device->flags, DEVICE_FLAGS_BUFFERED)) {
        BlockBuffer* blockBuffer = blockDevice->blockBuffer;
        for (int i = 0; i < blockBuffer->blockNum; ++i) {
            BlockBufferBlock* block = blockBuffer_pop(blockBuffer, INVALID_INDEX);
            if (block == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

            if (block->blockIndex != INVALID_INDEX && TEST_FLAGS(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY)) {
                device_rawWrite(device, block->blockIndex, block->data, 1);
                ERROR_GOTO_IF_ERROR(0); //TODO: May be this happens too frequently?

                CLEAR_FLAG_BACK(block->flags, BLOCK_BUFFER_BLOCK_FLAGS_DIRTY);
            }

            blockBuffer_push(blockBuffer, block->blockIndex, block);
            ERROR_GOTO_IF_ERROR(0);
        }
    }

    device_rawFlush(device);
    ERROR_GOTO_IF_ERROR(0);
    return;
    ERROR_FINAL_BEGIN(0);
}

void blockDevice_probePartitions(BlockDevice* blopckDevice) {
    void* buffer = memory_allocate(BLOCK_DEVICE_DEFAULT_BLOCK_SIZE);
    if (buffer == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    __blockDevice_doProbePartitions(blopckDevice, buffer);
    ERROR_GOTO_IF_ERROR(0);

    ERROR_FINAL_BEGIN(0);
    memory_free(buffer);
}

typedef struct {
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

BlockDevice* firstBootablePartition;    //TODO: Ugly, figure out a method to know which device we are booting from

static void __blockDevice_doProbePartitions(BlockDevice* blockDevice, void* buffer) {
    blockDevice_readBlocks(blockDevice, 0, buffer, 1);
    ERROR_GOTO_IF_ERROR(0);

    Device* device = &blockDevice->device;
    char nameBuffer[DEVICE_NAME_MAX_LENGTH + 1];
    BlockDevice* partitionDevice = NULL;
    if (PTR_TO_VALUE(16, buffer + 0x1FE) == 0xAA55) {
        for (int i = 0; i < 4; ++i) {
            __MBRpartitionEntry* entry = (__MBRpartitionEntry*)(buffer + 0x1BE + i * sizeof(__MBRpartitionEntry));
            if (entry->systemID == 0) {
                continue;
            }

            print_sprintf(nameBuffer, "%s-p%d", device->name, i);    //TODO: sprintf not working correctly

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
                    .operations     = &_blockDevice_partition_deviceOperations,
                    .specificInfo   = entry->sectorBegin
                },
            };

            partitionDevice = memory_allocate(sizeof(BlockDevice));
            if (partitionDevice == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

            blockDevice_initStruct(partitionDevice, &args);
            ERROR_GOTO_IF_ERROR(1);

            device_registerDevice(&partitionDevice->device);
            ERROR_GOTO_IF_ERROR(1);
            
            if (firstBootablePartition == NULL) {
                firstBootablePartition = partitionDevice;
            }
        }
    }

    return;
    ERROR_FINAL_BEGIN(0);
    return;
    ERROR_FINAL_BEGIN(1);
    if (partitionDevice != NULL) {
        memory_free(partitionDevice);
    }
    ERROR_GOTO(0);
}

static void __blockDevice_partition_read(Device* device, Index64 index, void* buffer, Size n) {
    blockDevice_readBlocks(HOST_POINTER(device->parent, BlockDevice, device), (Index64)device->specificInfo + index, buffer, n);
}

static void __blockDevice_partition_write(Device* device, Index64 index, const void* buffer, Size n) {
    blockDevice_writeBlocks(HOST_POINTER(device->parent, BlockDevice, device), (Index64)device->specificInfo + index, buffer, n);
}

static void __blockDevice_partition_flush(Device* device) {
    blockDevice_flush(HOST_POINTER(device->parent, BlockDevice, device));
}